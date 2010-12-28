//
//  Copyright (C) 2001,2002 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2001,2002 Constantin Kaplinsky.  All Rights Reserved.
//  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.io.*;
import java.lang.*;
import java.util.zip.*;

import java.util.Collections;

//
// VncCanvas is a subclass of Canvas which draws a VNC desktop on it.
//

class VncCanvas
	extends Canvas
	implements KeyListener, MouseListener, MouseMotionListener, MouseWheelListener {

	VncViewer viewer;
	RfbProto rfb;
	ColorModel cm8_256c, cm8_64c, cm8_8c, cm24;
	Color[] colors;
	int bytesPixel;

	Image memImage;
	Graphics memGraphics;

	Image rawPixelsImage;
	MemoryImageSource pixelsSource;
	byte[] pixels8;
	int[] pixels24;

	// Zlib encoder's data.
	byte[] zlibBuf;
	int zlibBufLen = 0;
	Inflater zlibInflater;

	// Tight encoder's data.
	final static int tightZlibBufferSize = 512;
	Inflater[] tightInflaters;

	// Since JPEG images are loaded asynchronously, we have to remember
	// their position in the framebuffer. Also, this jpegRect object is
	// used for synchronization between the rfbThread and a JVM's thread
	// which decodes and loads JPEG images.
	Rectangle jpegRect;

	// True if we process keyboard and mouse events.
	boolean inputEnabled;
	
	//
	// The constructor.
	//

	VncCanvas(VncViewer v) throws IOException {
		viewer = v;
		rfb = viewer.rfb;

		tightInflaters = new Inflater[4];

		// sf@2005 - Adding more color modes
		cm8_256c = new DirectColorModel(8, 7, (7 << 3), (3 << 6));
		cm8_64c = new DirectColorModel(8, (3 << 4), (3 << 2), (3 << 0));
		cm8_8c = new DirectColorModel(8, (1 << 2), (1 << 1), (1 << 0));
		
		cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);

    // kludge to not show any Java cursor in the canvas since we are
    // showing the soft cursor (should be a user setting...)
/*    Cursor dot = Toolkit.getDefaultToolkit().createCustomCursor(
        Toolkit.getDefaultToolkit().createImage(new byte[4]), new Point(0,0),
        "dot");
    this.setCursor(dot);*/

    // while we are at it... get rid of the keyboard traversals that
    // make it so we can't type a Tab character:
    this.setFocusTraversalKeys(KeyboardFocusManager.FORWARD_TRAVERSAL_KEYS,
        Collections.EMPTY_SET);
    this.setFocusTraversalKeys(KeyboardFocusManager.BACKWARD_TRAVERSAL_KEYS,
        Collections.EMPTY_SET);

		colors = new Color[256];
		// sf@2005 - Now Default
		for (int i = 0; i < 256; i++)
			colors[i] = new Color(cm8_256c.getRGB(i));

		setPixelFormat();

		inputEnabled = false;
		if (!viewer.options.viewOnly)
			enableInput(true);

		// Keyboard listener is enabled even in view-only mode, to catch
		// 'r' or 'R' key presses used to request screen update.
		addKeyListener(this);
	}

	//
	// Callback methods to determine geometry of our Component.
	//

	public Dimension getPreferredSize() {
		return new Dimension(rfb.framebufferWidth, rfb.framebufferHeight);
	}

	public Dimension getMinimumSize() {
		return new Dimension(rfb.framebufferWidth, rfb.framebufferHeight);
	}

	public Dimension getMaximumSize() {
		return new Dimension(rfb.framebufferWidth, rfb.framebufferHeight);
	}

	//
	// All painting is performed here.
	//

	public void update(Graphics g) {
		paint(g);
	}

	public void paint(Graphics g) {
		synchronized (memImage) {
			g.drawImage(memImage, 0, 0, null);
		}
		if (showSoftCursor) {
			int x0 = cursorX - hotX, y0 = cursorY - hotY;
			Rectangle r = new Rectangle(x0, y0, cursorWidth, cursorHeight);
			if (r.intersects(g.getClipBounds())) {
				g.drawImage(softCursor, x0, y0, null);
			}
		}
	}

	//
	// Override the ImageObserver interface method to handle drawing of
	// JPEG-encoded data.
	//

	public boolean imageUpdate(
		Image img,
		int infoflags,
		int x,
		int y,
		int width,
		int height) {
		if ((infoflags & (ALLBITS | ABORT)) == 0) {
			return true; // We need more image data.
		} else {
			// If the whole image is available, draw it now.
			if ((infoflags & ALLBITS) != 0) {
				if (jpegRect != null) {
					synchronized (jpegRect) {
						memGraphics.drawImage(
							img,
							jpegRect.x,
							jpegRect.y,
							null);
						scheduleRepaint(
							jpegRect.x,
							jpegRect.y,
							jpegRect.width,
							jpegRect.height);
						jpegRect.notify();
					}
				}
			}
			return false; // All image data was processed.
		}
	}

	//
	// Start/stop receiving mouse events. Keyboard events are received
	// even in view-only mode, because we want to map the 'r' key to the
	// screen refreshing function.
	//

	public synchronized void enableInput(boolean enable) {
		if (enable && !inputEnabled) {
			inputEnabled = true;
			addMouseListener(this);
			addMouseMotionListener(this);
			addMouseWheelListener(this);
			if (viewer.showControls) {
				viewer.buttonPanel.enableRemoteAccessControls(true);
			}
		} else if (!enable && inputEnabled) {
			inputEnabled = false;
			removeMouseListener(this);
			removeMouseMotionListener(this);
			removeMouseWheelListener(this);
			if (viewer.showControls) {
				viewer.buttonPanel.enableRemoteAccessControls(false);
			}
		}
	}

	
	public void setPixelFormat() throws IOException {
		// sf@2005 - Adding more color modes
		if (viewer.options.eightBitColors > 0)
		{
			viewer.options.oldEightBitColors = viewer.options.eightBitColors;
			switch (viewer.options.eightBitColors)
			{
				case 1: // 256
					for (int i = 0; i < 256; i++)
						colors[i] = new Color(cm8_256c.getRGB(i));					
					rfb.writeSetPixelFormat(8, 8, false, true, 7, 7, 3, 0, 3, 6, false);
					break;
				case 2: // 64
					for (int i = 0; i < 256; i++)
						colors[i] = new Color(cm8_64c.getRGB(i));					
					rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 4, 2, 0, false);
					break;
				case 3: // 8
					for (int i = 0; i < 256; i++)
						colors[i] = new Color(cm8_8c.getRGB(i));					
					rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 2, 1, 0, false);
					break;
				case 4: // 4 (Grey)
					for (int i = 0; i < 256; i++)
						colors[i] = new Color(cm8_64c.getRGB(i));					
					rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 4, 2, 0, true);
					break;
				case 5: // 2 (B&W)
					for (int i = 0; i < 256; i++)
						colors[i] = new Color(cm8_8c.getRGB(i));					
					rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 2, 1, 0, true);
					break;
			}
			bytesPixel = 1;
		}
		else 
		{
			rfb.writeSetPixelFormat(
				32,
				24,
				false,
				true,
				255,
				255,
				255,
				16,
				8,
				0,
				false);
			bytesPixel = 4;
		}
		updateFramebufferSize();
	}

	void updateFramebufferSize() {

		// Useful shortcuts.
		int fbWidth = rfb.framebufferWidth;
		int fbHeight = rfb.framebufferHeight;

		// Create new off-screen image either if it does not exist, or if
		// its geometry should be changed. It's not necessary to replace
		// existing image if only pixel format should be changed.
		if (memImage == null) {
			memImage = viewer.createImage(fbWidth, fbHeight);
			memGraphics = memImage.getGraphics();
		} else if (
			memImage.getWidth(null) != fbWidth
				|| memImage.getHeight(null) != fbHeight) {
			synchronized (memImage) {
				memImage = viewer.createImage(fbWidth, fbHeight);
				memGraphics = memImage.getGraphics();
			}
		}

		// Images with raw pixels should be re-allocated on every change
		// of geometry or pixel format.
		if (bytesPixel == 1) {
			pixels24 = null;
			pixels8 = new byte[fbWidth * fbHeight];

			// sf@2005
			ColorModel cml = cm8_8c;
			
			//	 sf@2005
			switch (viewer.options.eightBitColors)
			{
				case 1:
					cml = cm8_256c;
					break;
					
				case 2:
				case 4:
					cml = cm8_64c;
					break;
				case 3:
				case 5:
					cml = cm8_8c;
					break;
			}
			
			pixelsSource =
				new MemoryImageSource(
					fbWidth,
					fbHeight,
					cml,
					pixels8,
					0,
					fbWidth);
		} else {
			pixels8 = null;
			pixels24 = new int[fbWidth * fbHeight];

			pixelsSource =
				new MemoryImageSource(
					fbWidth,
					fbHeight,
					cm24,
					pixels24,
					0,
					fbWidth);
		}
		pixelsSource.setAnimated(true);
		rawPixelsImage = createImage(pixelsSource);

		// Update the size of desktop containers.
		if (viewer.inSeparateFrame) {
			if (viewer.desktopScrollPane != null)
				resizeDesktopFrame();
		} else {
			setSize(fbWidth, fbHeight);
		}
	}

	void resizeDesktopFrame() {
		setSize(rfb.framebufferWidth, rfb.framebufferHeight);

		// FIXME: Find a better way to determine correct size of a
		// ScrollPane.  -- const
		Insets insets = viewer.desktopScrollPane.getInsets();
		viewer.desktopScrollPane.setSize(
			rfb.framebufferWidth + 2 * Math.min(insets.left, insets.right),
			rfb.framebufferHeight + 2 * Math.min(insets.top, insets.bottom));

		viewer.vncFrame.pack();

		// Try to limit the frame size to the screen size.
		Dimension screenSize = viewer.vncFrame.getToolkit().getScreenSize();
		Dimension frameSize = viewer.vncFrame.getSize();
		Dimension newSize = frameSize;
		boolean needToResizeFrame = false;
		if (frameSize.height > screenSize.height) {
			newSize.height = screenSize.height;
			needToResizeFrame = true;
		}
		if (frameSize.width > screenSize.width) {
			newSize.width = screenSize.width;
			needToResizeFrame = true;
		}
		if (needToResizeFrame) {
			viewer.vncFrame.setSize(newSize);
		}

		viewer.desktopScrollPane.doLayout();
	}

	//
	// processNormalProtocol() - executed by the rfbThread to deal with the
	// RFB socket.
	//

	public void processNormalProtocol() throws Exception {

		// Start/stop session recording if necessary.
		viewer.checkRecordingStatus();

		rfb.writeFramebufferUpdateRequest(
			0,
			0,
			rfb.framebufferWidth,
			rfb.framebufferHeight,
			false);

		//
		// main dispatch loop
		//

		while (true) {
			// Read message type from the server.
			int msgType = rfb.readServerMessageType();

			// Process the message depending on its type.
			switch (msgType) {
				case RfbProto.FramebufferUpdate :
					rfb.readFramebufferUpdate();

					for (int i = 0; i < rfb.updateNRects; i++) {
						rfb.readFramebufferUpdateRectHdr();
						int rx = rfb.updateRectX, ry = rfb.updateRectY;
						int rw = rfb.updateRectW, rh = rfb.updateRectH;

						if (rfb.updateRectEncoding == rfb.EncodingLastRect)
							break;

						if (rfb.updateRectEncoding == rfb.EncodingNewFBSize) {
							rfb.setFramebufferSize(rw, rh);
							updateFramebufferSize();
							break;
						}

						if (rfb.updateRectEncoding == rfb.EncodingXCursor
							|| rfb.updateRectEncoding == rfb.EncodingRichCursor) {
							handleCursorShapeUpdate(
								rfb.updateRectEncoding,
								rx,
								ry,
								rw,
								rh);
							continue;
						}

						switch (rfb.updateRectEncoding) {
							case RfbProto.EncodingRaw :
								handleRawRect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingCopyRect :
								handleCopyRect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingRRE :
								handleRRERect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingCoRRE :
								handleCoRRERect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingHextile :
								handleHextileRect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingZlib :
								handleZlibRect(rx, ry, rw, rh);
								break;
							case RfbProto.EncodingTight :
								handleTightRect(rx, ry, rw, rh);
								break;
							// marscha - PointerPos
							case RfbProto.EncodingPointerPos :
								handleCursorPosUpdate(rx, ry);
								break;
							default :
								throw new Exception(
									"Unknown RFB rectangle encoding "
										+ rfb.updateRectEncoding);
						}
					}

					boolean fullUpdateNeeded = false;

					// Start/stop session recording if necessary. Request full
					// update if a new session file was opened.
					if (viewer.checkRecordingStatus())
						fullUpdateNeeded = true;

					// Defer framebuffer update request if necessary. But wake up
					// immediately on keyboard or mouse event.
					if (viewer.deferUpdateRequests > 0) {
						synchronized (rfb) {
							try {
								rfb.wait(viewer.deferUpdateRequests);
							} catch (InterruptedException e) {
							}
						}
					}

					// Before requesting framebuffer update, check if the pixel
					// format should be changed. If it should, request full update
					// instead of an incremental one.
					if ((viewer.options.eightBitColors > 0) && (bytesPixel != 1)
						||
						(viewer.options.eightBitColors == 0) && (bytesPixel == 1)
						||
						(viewer.options.eightBitColors != viewer.options.oldEightBitColors)
						)
					{
						setPixelFormat();
						fullUpdateNeeded = true;
					}

					rfb.writeFramebufferUpdateRequest(
						0,
						0,
						rfb.framebufferWidth,
						rfb.framebufferHeight,
						!fullUpdateNeeded);

					break;

				case RfbProto.SetColourMapEntries :
					throw new Exception("Can't handle SetColourMapEntries message");

				case RfbProto.Bell :
					Toolkit.getDefaultToolkit().beep();
					break;

				case RfbProto.ServerCutText :
					String s = rfb.readServerCutText();
					viewer.clipboard.setCutText(s);
					break;

				case RfbProto.rfbFileTransfer :
					viewer.rfb.readRfbFileTransferMsg();
					break;

				default :
					throw new Exception("Unknown RFB message type " + msgType);
			}
		}
	}

	//
	// Handle a raw rectangle. The second form with paint==false is used
	// by the Hextile decoder for raw-encoded tiles.
	//

	void handleRawRect(int x, int y, int w, int h) throws IOException {
		handleRawRect(x, y, w, h, true);
	}

	void handleRawRect(int x, int y, int w, int h, boolean paint)
		throws IOException {

		if (bytesPixel == 1) {
			for (int dy = y; dy < y + h; dy++) {
				rfb.is.readFully(pixels8, dy * rfb.framebufferWidth + x, w);
				if (rfb.rec != null) {
					rfb.rec.write(pixels8, dy * rfb.framebufferWidth + x, w);
				}
			}
		} else {
			byte[] buf = new byte[w * 4];
			int i, offset;
			for (int dy = y; dy < y + h; dy++) {
				rfb.is.readFully(buf);
				if (rfb.rec != null) {
					rfb.rec.write(buf);
				}
				offset = dy * rfb.framebufferWidth + x;
				for (i = 0; i < w; i++) {
					pixels24[offset + i] =
						(buf[i * 4 + 2] & 0xFF)
							<< 16 | (buf[i * 4 + 1] & 0xFF)
							<< 8 | (buf[i * 4] & 0xFF);
				}
			}
		}

		handleUpdatedPixels(x, y, w, h);
		if (paint)
			scheduleRepaint(x, y, w, h);
	}

	//
	// Handle a CopyRect rectangle.
	//

	void handleCopyRect(int x, int y, int w, int h) throws IOException {

		rfb.readCopyRect();
		memGraphics.copyArea(
			rfb.copyRectSrcX,
			rfb.copyRectSrcY,
			w,
			h,
			x - rfb.copyRectSrcX,
			y - rfb.copyRectSrcY);

		scheduleRepaint(x, y, w, h);
	}

	//
	// Handle an RRE-encoded rectangle.
	//

	void handleRRERect(int x, int y, int w, int h) throws IOException {

		int nSubrects = rfb.is.readInt();

		byte[] bg_buf = new byte[bytesPixel];
		rfb.is.readFully(bg_buf);
		Color pixel;
		if (bytesPixel == 1)
		{
			pixel = colors[bg_buf[0] & 0xFF];
		}
		else
		{
			pixel = new Color(bg_buf[2] & 0xFF, bg_buf[1] & 0xFF, bg_buf[0] & 0xFF);
		}
		memGraphics.setColor(pixel);
		memGraphics.fillRect(x, y, w, h);

		byte[] buf = new byte[nSubrects * (bytesPixel + 8)];
		rfb.is.readFully(buf);
		DataInputStream ds = new DataInputStream(new ByteArrayInputStream(buf));

		if (rfb.rec != null) {
			rfb.rec.writeIntBE(nSubrects);
			rfb.rec.write(bg_buf);
			rfb.rec.write(buf);
		}

		int sx, sy, sw, sh;

		for (int j = 0; j < nSubrects; j++) {
			if (bytesPixel == 1) {
				pixel = colors[ds.readUnsignedByte()];
			} else {
				ds.skip(4);
				pixel =
					new Color(
						buf[j * 12 + 2] & 0xFF,
						buf[j * 12 + 1] & 0xFF,
						buf[j * 12] & 0xFF);
			}
			sx = x + ds.readUnsignedShort();
			sy = y + ds.readUnsignedShort();
			sw = ds.readUnsignedShort();
			sh = ds.readUnsignedShort();

			memGraphics.setColor(pixel);
			memGraphics.fillRect(sx, sy, sw, sh);
		}

		scheduleRepaint(x, y, w, h);
	}

	//
	// Handle a CoRRE-encoded rectangle.
	//

	void handleCoRRERect(int x, int y, int w, int h) throws IOException {
		int nSubrects = rfb.is.readInt();

		byte[] bg_buf = new byte[bytesPixel];
		rfb.is.readFully(bg_buf);
		Color pixel;
		if (bytesPixel == 1) {
			pixel = colors[bg_buf[0] & 0xFF];
		} else {
			pixel =
				new Color(bg_buf[2] & 0xFF, bg_buf[1] & 0xFF, bg_buf[0] & 0xFF);
		}
		memGraphics.setColor(pixel);
		memGraphics.fillRect(x, y, w, h);

		byte[] buf = new byte[nSubrects * (bytesPixel + 4)];
		rfb.is.readFully(buf);

		if (rfb.rec != null) {
			rfb.rec.writeIntBE(nSubrects);
			rfb.rec.write(bg_buf);
			rfb.rec.write(buf);
		}

		int sx, sy, sw, sh;
		int i = 0;

		for (int j = 0; j < nSubrects; j++) {
			if (bytesPixel == 1) {
				pixel = colors[buf[i++] & 0xFF];
			} else {
				pixel =
					new Color(
						buf[i + 2] & 0xFF,
						buf[i + 1] & 0xFF,
						buf[i] & 0xFF);
				i += 4;
			}
			sx = x + (buf[i++] & 0xFF);
			sy = y + (buf[i++] & 0xFF);
			sw = buf[i++] & 0xFF;
			sh = buf[i++] & 0xFF;

			memGraphics.setColor(pixel);
			memGraphics.fillRect(sx, sy, sw, sh);
		}

		scheduleRepaint(x, y, w, h);
	}

	//
	// Handle a Hextile-encoded rectangle.
	//

	// These colors should be kept between handleHextileSubrect() calls.
	private Color hextile_bg, hextile_fg;

	void handleHextileRect(int x, int y, int w, int h) throws IOException {

		hextile_bg = new Color(0);
		hextile_fg = new Color(0);

		for (int ty = y; ty < y + h; ty += 16) {
			int th = 16;
			if (y + h - ty < 16)
				th = y + h - ty;

			for (int tx = x; tx < x + w; tx += 16) {
				int tw = 16;
				if (x + w - tx < 16)
					tw = x + w - tx;

				handleHextileSubrect(tx, ty, tw, th);
			}

			// Finished with a row of tiles, now let's show it.
			scheduleRepaint(x, y, w, h);
		}
	}

	//
	// Handle one tile in the Hextile-encoded data.
	//

	void handleHextileSubrect(int tx, int ty, int tw, int th)
		throws IOException {

		int subencoding = rfb.is.readUnsignedByte();
		if (rfb.rec != null) {
			rfb.rec.writeByte(subencoding);
		}

		// Is it a raw-encoded sub-rectangle?
		if ((subencoding & rfb.HextileRaw) != 0) {
			handleRawRect(tx, ty, tw, th, false);
			return;
		}

		// Read and draw the background if specified.
		byte[] cbuf = new byte[bytesPixel];
		if ((subencoding & rfb.HextileBackgroundSpecified) != 0) {
			rfb.is.readFully(cbuf);
			if (bytesPixel == 1) {
				hextile_bg = colors[cbuf[0] & 0xFF];
			} else {
				hextile_bg =
					new Color(cbuf[2] & 0xFF, cbuf[1] & 0xFF, cbuf[0] & 0xFF);
			}
			if (rfb.rec != null) {
				rfb.rec.write(cbuf);
			}
		}
		memGraphics.setColor(hextile_bg);
		memGraphics.fillRect(tx, ty, tw, th);

		// Read the foreground color if specified.
		if ((subencoding & rfb.HextileForegroundSpecified) != 0) {
			rfb.is.readFully(cbuf);
			if (bytesPixel == 1) {
				hextile_fg = colors[cbuf[0] & 0xFF];
			} else {
				hextile_fg =
					new Color(cbuf[2] & 0xFF, cbuf[1] & 0xFF, cbuf[0] & 0xFF);
			}
			if (rfb.rec != null) {
				rfb.rec.write(cbuf);
			}
		}

		// Done with this tile if there is no sub-rectangles.
		if ((subencoding & rfb.HextileAnySubrects) == 0)
			return;

		int nSubrects = rfb.is.readUnsignedByte();
		int bufsize = nSubrects * 2;
		if ((subencoding & rfb.HextileSubrectsColoured) != 0) {
			bufsize += nSubrects * bytesPixel;
		}
		byte[] buf = new byte[bufsize];
		rfb.is.readFully(buf);
		if (rfb.rec != null) {
			rfb.rec.writeByte(nSubrects);
			rfb.rec.write(buf);
		}

		int b1, b2, sx, sy, sw, sh;
		int i = 0;

		if ((subencoding & rfb.HextileSubrectsColoured) == 0) {

			// Sub-rectangles are all of the same color.
			memGraphics.setColor(hextile_fg);
			for (int j = 0; j < nSubrects; j++) {
				b1 = buf[i++] & 0xFF;
				b2 = buf[i++] & 0xFF;
				sx = tx + (b1 >> 4);
				sy = ty + (b1 & 0xf);
				sw = (b2 >> 4) + 1;
				sh = (b2 & 0xf) + 1;
				memGraphics.fillRect(sx, sy, sw, sh);
			}
		} else if (bytesPixel == 1) {

			// BGR233 (8-bit color) version for colored sub-rectangles.
			for (int j = 0; j < nSubrects; j++) {
				hextile_fg = colors[buf[i++] & 0xFF];
				b1 = buf[i++] & 0xFF;
				b2 = buf[i++] & 0xFF;
				sx = tx + (b1 >> 4);
				sy = ty + (b1 & 0xf);
				sw = (b2 >> 4) + 1;
				sh = (b2 & 0xf) + 1;
				memGraphics.setColor(hextile_fg);
				memGraphics.fillRect(sx, sy, sw, sh);
			}

		} else {

			// Full-color (24-bit) version for colored sub-rectangles.
			for (int j = 0; j < nSubrects; j++) {
				hextile_fg =
					new Color(
						buf[i + 2] & 0xFF,
						buf[i + 1] & 0xFF,
						buf[i] & 0xFF);
				i += 4;
				b1 = buf[i++] & 0xFF;
				b2 = buf[i++] & 0xFF;
				sx = tx + (b1 >> 4);
				sy = ty + (b1 & 0xf);
				sw = (b2 >> 4) + 1;
				sh = (b2 & 0xf) + 1;
				memGraphics.setColor(hextile_fg);
				memGraphics.fillRect(sx, sy, sw, sh);
			}

		}
	}

	//
	// Handle a Zlib-encoded rectangle.
	//

	void handleZlibRect(int x, int y, int w, int h) throws Exception {

		int nBytes = rfb.is.readInt();

		if (zlibBuf == null || zlibBufLen < nBytes) {
			zlibBufLen = nBytes * 2;
			zlibBuf = new byte[zlibBufLen];
		}

		rfb.is.readFully(zlibBuf, 0, nBytes);

		if (rfb.rec != null && rfb.recordFromBeginning) {
			rfb.rec.writeIntBE(nBytes);
			rfb.rec.write(zlibBuf, 0, nBytes);
		}

		if (zlibInflater == null) {
			zlibInflater = new Inflater();
		}
		zlibInflater.setInput(zlibBuf, 0, nBytes);

		if (bytesPixel == 1) {
			for (int dy = y; dy < y + h; dy++) {
				zlibInflater.inflate(pixels8, dy * rfb.framebufferWidth + x, w);
				if (rfb.rec != null && !rfb.recordFromBeginning)
					rfb.rec.write(pixels8, dy * rfb.framebufferWidth + x, w);
			}
		} else {
			byte[] buf = new byte[w * 4];
			int i, offset;
			for (int dy = y; dy < y + h; dy++) {
				zlibInflater.inflate(buf);
				offset = dy * rfb.framebufferWidth + x;
				for (i = 0; i < w; i++) {
					pixels24[offset + i] =
						(buf[i * 4 + 2] & 0xFF)
							<< 16 | (buf[i * 4 + 1] & 0xFF)
							<< 8 | (buf[i * 4] & 0xFF);
				}
				if (rfb.rec != null && !rfb.recordFromBeginning)
					rfb.rec.write(buf);
			}
		}

		handleUpdatedPixels(x, y, w, h);
		scheduleRepaint(x, y, w, h);
	}

	//
	// Handle a Tight-encoded rectangle.
	//

	void handleTightRect(int x, int y, int w, int h) throws Exception {

		int comp_ctl = rfb.is.readUnsignedByte();
		if (rfb.rec != null) {
			if (rfb.recordFromBeginning
				|| comp_ctl == (rfb.TightFill << 4)
				|| comp_ctl == (rfb.TightJpeg << 4)) {
				// Send data exactly as received.
				rfb.rec.writeByte(comp_ctl);
			} else {
				// Tell the decoder to flush each of the four zlib streams.
				rfb.rec.writeByte(comp_ctl | 0x0F);
			}
		}

		// Flush zlib streams if we are told by the server to do so.
		for (int stream_id = 0; stream_id < 4; stream_id++) {
			if ((comp_ctl & 1) != 0 && tightInflaters[stream_id] != null) {
				tightInflaters[stream_id] = null;
			}
			comp_ctl >>= 1;
		}

		// Check correctness of subencoding value.
		if (comp_ctl > rfb.TightMaxSubencoding) {
			throw new Exception("Incorrect tight subencoding: " + comp_ctl);
		}

		// Handle solid-color rectangles.
		if (comp_ctl == rfb.TightFill) {

			if (bytesPixel == 1) {
				int idx = rfb.is.readUnsignedByte();
				memGraphics.setColor(colors[idx]);
				if (rfb.rec != null) {
					rfb.rec.writeByte(idx);
				}
			} else {
				byte[] buf = new byte[3];
				rfb.is.readFully(buf);
				if (rfb.rec != null) {
					rfb.rec.write(buf);
				}
				Color bg =
					new Color(
						0xFF000000 | (buf[0] & 0xFF)
							<< 16 | (buf[1] & 0xFF)
							<< 8 | (buf[2] & 0xFF));
				memGraphics.setColor(bg);
			}
			memGraphics.fillRect(x, y, w, h);
			scheduleRepaint(x, y, w, h);
			return;

		}

		if (comp_ctl == rfb.TightJpeg) {

			// Read JPEG data.
			byte[] jpegData = new byte[rfb.readCompactLen()];
			rfb.is.readFully(jpegData);
			if (rfb.rec != null) {
				if (!rfb.recordFromBeginning) {
					rfb.recordCompactLen(jpegData.length);
				}
				rfb.rec.write(jpegData);
			}

			// Create an Image object from the JPEG data.
			Image jpegImage = Toolkit.getDefaultToolkit().createImage(jpegData);

			// Remember the rectangle where the image should be drawn.
			jpegRect = new Rectangle(x, y, w, h);

			// Let the imageUpdate() method do the actual drawing, here just
			// wait until the image is fully loaded and drawn.
			synchronized (jpegRect) {
				Toolkit.getDefaultToolkit().prepareImage(
					jpegImage,
					-1,
					-1,
					this);
				try {
					// Wait no longer than three seconds.
					jpegRect.wait(3000);
				} catch (InterruptedException e) {
					throw new Exception("Interrupted while decoding JPEG image");
				}
			}

			// Done, jpegRect is not needed any more.
			jpegRect = null;
			return;

		}

		// Read filter id and parameters.
		int numColors = 0, rowSize = w;
		byte[] palette8 = new byte[2];
		int[] palette24 = new int[256];
		boolean useGradient = false;
		if ((comp_ctl & rfb.TightExplicitFilter) != 0) {
			int filter_id = rfb.is.readUnsignedByte();
			if (rfb.rec != null) {
				rfb.rec.writeByte(filter_id);
			}
			if (filter_id == rfb.TightFilterPalette) {
				numColors = rfb.is.readUnsignedByte() + 1;
				if (rfb.rec != null) {
					rfb.rec.writeByte(numColors - 1);
				}
				if (bytesPixel == 1) {
					if (numColors != 2) {
						throw new Exception(
							"Incorrect tight palette size: " + numColors);
					}
					rfb.is.readFully(palette8);
					if (rfb.rec != null) {
						rfb.rec.write(palette8);
					}
				} else {
					byte[] buf = new byte[numColors * 3];
					rfb.is.readFully(buf);
					if (rfb.rec != null) {
						rfb.rec.write(buf);
					}
					for (int i = 0; i < numColors; i++) {
						palette24[i] =
							((buf[i * 3] & 0xFF)
								<< 16 | (buf[i * 3 + 1] & 0xFF)
								<< 8 | (buf[i * 3 + 2] & 0xFF));
					}
				}
				if (numColors == 2)
					rowSize = (w + 7) / 8;
			} else if (filter_id == rfb.TightFilterGradient) {
				useGradient = true;
			} else if (filter_id != rfb.TightFilterCopy) {
				throw new Exception("Incorrect tight filter id: " + filter_id);
			}
		}
		if (numColors == 0 && bytesPixel == 4)
			rowSize *= 3;

		// Read, optionally uncompress and decode data.
		int dataSize = h * rowSize;
		if (dataSize < rfb.TightMinToCompress) {
			// Data size is small - not compressed with zlib.
			if (numColors != 0) {
				// Indexed colors.
				byte[] indexedData = new byte[dataSize];
				rfb.is.readFully(indexedData);
				if (rfb.rec != null) {
					rfb.rec.write(indexedData);
				}
				if (numColors == 2) {
					// Two colors.
					if (bytesPixel == 1) {
						decodeMonoData(x, y, w, h, indexedData, palette8);
					} else {
						decodeMonoData(x, y, w, h, indexedData, palette24);
					}
				} else {
					// 3..255 colors (assuming bytesPixel == 4).
					int i = 0;
					for (int dy = y; dy < y + h; dy++) {
						for (int dx = x; dx < x + w; dx++) {
							pixels24[dy * rfb.framebufferWidth + dx] =
								palette24[indexedData[i++] & 0xFF];
						}
					}
				}
			} else if (useGradient) {
				// "Gradient"-processed data
				byte[] buf = new byte[w * h * 3];
				rfb.is.readFully(buf);
				if (rfb.rec != null) {
					rfb.rec.write(buf);
				}
				decodeGradientData(x, y, w, h, buf);
			} else {
				// Raw truecolor data.
				if (bytesPixel == 1) {
					for (int dy = y; dy < y + h; dy++) {
						rfb.is.readFully(
							pixels8,
							dy * rfb.framebufferWidth + x,
							w);
						if (rfb.rec != null) {
							rfb.rec.write(
								pixels8,
								dy * rfb.framebufferWidth + x,
								w);
						}
					}
				} else {
					byte[] buf = new byte[w * 3];
					int i, offset;
					for (int dy = y; dy < y + h; dy++) {
						rfb.is.readFully(buf);
						if (rfb.rec != null) {
							rfb.rec.write(buf);
						}
						offset = dy * rfb.framebufferWidth + x;
						for (i = 0; i < w; i++) {
							pixels24[offset + i] =
								(buf[i * 3] & 0xFF)
									<< 16 | (buf[i * 3 + 1] & 0xFF)
									<< 8 | (buf[i * 3 + 2] & 0xFF);
						}
					}
				}
			}
		} else {
			// Data was compressed with zlib.
			int zlibDataLen = rfb.readCompactLen();
			byte[] zlibData = new byte[zlibDataLen];
			rfb.is.readFully(zlibData);
			if (rfb.rec != null && rfb.recordFromBeginning) {
				rfb.rec.write(zlibData);
			}
			int stream_id = comp_ctl & 0x03;
			if (tightInflaters[stream_id] == null) {
				tightInflaters[stream_id] = new Inflater();
			}
			Inflater myInflater = tightInflaters[stream_id];
			myInflater.setInput(zlibData);
			byte[] buf = new byte[dataSize];
			myInflater.inflate(buf);
			if (rfb.rec != null && !rfb.recordFromBeginning) {
				rfb.recordCompressedData(buf);
			}

			if (numColors != 0) {
				// Indexed colors.
				if (numColors == 2) {
					// Two colors.
					if (bytesPixel == 1) {
						decodeMonoData(x, y, w, h, buf, palette8);
					} else {
						decodeMonoData(x, y, w, h, buf, palette24);
					}
				} else {
					// More than two colors (assuming bytesPixel == 4).
					int i = 0;
					for (int dy = y; dy < y + h; dy++) {
						for (int dx = x; dx < x + w; dx++) {
							pixels24[dy * rfb.framebufferWidth + dx] =
								palette24[buf[i++] & 0xFF];
						}
					}
				}
			} else if (useGradient) {
				// Compressed "Gradient"-filtered data (assuming bytesPixel == 4).
				decodeGradientData(x, y, w, h, buf);
			} else {
				// Compressed truecolor data.
				if (bytesPixel == 1) {
					int destOffset = y * rfb.framebufferWidth + x;
					for (int dy = 0; dy < h; dy++) {
						System.arraycopy(buf, dy * w, pixels8, destOffset, w);
						destOffset += rfb.framebufferWidth;
					}
				} else {
					int srcOffset = 0;
					int destOffset, i;
					for (int dy = 0; dy < h; dy++) {
						myInflater.inflate(buf);
						destOffset = (y + dy) * rfb.framebufferWidth + x;
						for (i = 0; i < w; i++) {
							pixels24[destOffset + i] =
								(buf[srcOffset] & 0xFF)
									<< 16 | (buf[srcOffset + 1] & 0xFF)
									<< 8 | (buf[srcOffset + 2] & 0xFF);
							srcOffset += 3;
						}
					}
				}
			}
		}

		handleUpdatedPixels(x, y, w, h);
		scheduleRepaint(x, y, w, h);
	}

	//
	// Decode 1bpp-encoded bi-color rectangle (8-bit and 24-bit versions).
	//

	void decodeMonoData(
		int x,
		int y,
		int w,
		int h,
		byte[] src,
		byte[] palette) {

		int dx, dy, n;
		int i = y * rfb.framebufferWidth + x;
		int rowBytes = (w + 7) / 8;
		byte b;

		for (dy = 0; dy < h; dy++) {
			for (dx = 0; dx < w / 8; dx++) {
				b = src[dy * rowBytes + dx];
				for (n = 7; n >= 0; n--)
					pixels8[i++] = palette[b >> n & 1];
			}
			for (n = 7; n >= 8 - w % 8; n--) {
				pixels8[i++] = palette[src[dy * rowBytes + dx] >> n & 1];
			}
			i += (rfb.framebufferWidth - w);
		}
	}

	void decodeMonoData(
		int x,
		int y,
		int w,
		int h,
		byte[] src,
		int[] palette) {

		int dx, dy, n;
		int i = y * rfb.framebufferWidth + x;
		int rowBytes = (w + 7) / 8;
		byte b;

		for (dy = 0; dy < h; dy++) {
			for (dx = 0; dx < w / 8; dx++) {
				b = src[dy * rowBytes + dx];
				for (n = 7; n >= 0; n--)
					pixels24[i++] = palette[b >> n & 1];
			}
			for (n = 7; n >= 8 - w % 8; n--) {
				pixels24[i++] = palette[src[dy * rowBytes + dx] >> n & 1];
			}
			i += (rfb.framebufferWidth - w);
		}
	}

	//
	// Decode data processed with the "Gradient" filter.
	//

	void decodeGradientData(int x, int y, int w, int h, byte[] buf) {

		int dx, dy, c;
		byte[] prevRow = new byte[w * 3];
		byte[] thisRow = new byte[w * 3];
		byte[] pix = new byte[3];
		int[] est = new int[3];

		int offset = y * rfb.framebufferWidth + x;

		for (dy = 0; dy < h; dy++) {

			/* First pixel in a row */
			for (c = 0; c < 3; c++) {
				pix[c] = (byte) (prevRow[c] + buf[dy * w * 3 + c]);
				thisRow[c] = pix[c];
			}
			pixels24[offset++] =
				(pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF);

			/* Remaining pixels of a row */
			for (dx = 1; dx < w; dx++) {
				for (c = 0; c < 3; c++) {
					est[c] =
						((prevRow[dx * 3 + c] & 0xFF)
							+ (pix[c] & 0xFF)
							- (prevRow[(dx - 1) * 3 + c] & 0xFF));
					if (est[c] > 0xFF) {
						est[c] = 0xFF;
					} else if (est[c] < 0x00) {
						est[c] = 0x00;
					}
					pix[c] = (byte) (est[c] + buf[(dy * w + dx) * 3 + c]);
					thisRow[dx * 3 + c] = pix[c];
				}
				pixels24[offset++] =
					(pix[0] & 0xFF)
						<< 16 | (pix[1] & 0xFF)
						<< 8 | (pix[2] & 0xFF);
			}

			System.arraycopy(thisRow, 0, prevRow, 0, w * 3);
			offset += (rfb.framebufferWidth - w);
		}
	}

	//
	// Display newly updated area of pixels.
	//

	void handleUpdatedPixels(int x, int y, int w, int h) {

		// Draw updated pixels of the off-screen image.
		pixelsSource.newPixels(x, y, w, h);
		memGraphics.setClip(x, y, w, h);
		memGraphics.drawImage(rawPixelsImage, 0, 0, null);
		memGraphics.setClip(0, 0, rfb.framebufferWidth, rfb.framebufferHeight);
	}

	//
	// Tell JVM to repaint specified desktop area.
	//

	void scheduleRepaint(int x, int y, int w, int h) {
		// Request repaint, deferred if necessary.
		repaint(viewer.deferScreenUpdates, x, y, w, h);
	}

	//
	// Handle events.
	//

	public void keyPressed(KeyEvent evt) {
		processLocalKeyEvent(evt);
	}
	public void keyReleased(KeyEvent evt) {
		processLocalKeyEvent(evt);
	}
	public void keyTyped(KeyEvent evt) {
		evt.consume();
	}

	public void mousePressed(MouseEvent evt) {
		processLocalMouseEvent(evt, false);
	}
	public void mouseReleased(MouseEvent evt) {
		processLocalMouseEvent(evt, false);
	}
	public void mouseMoved(MouseEvent evt) {
		processLocalMouseEvent(evt, true);
	}
	public void mouseDragged(MouseEvent evt) {
		processLocalMouseEvent(evt, true);
	}
	public void mouseWheelMoved(MouseWheelEvent evt) {
		processLocalMouseWheelEvent(evt);
	}

	public void processLocalKeyEvent(KeyEvent evt) {
		if (viewer.rfb != null && rfb.inNormalProtocol) {
			if (!inputEnabled) {
				if ((evt.getKeyChar() == 'r' || evt.getKeyChar() == 'R')
					&& evt.getID() == KeyEvent.KEY_PRESSED) {
					// Request screen update.
					try {
						rfb.writeFramebufferUpdateRequest(
							0,
							0,
							rfb.framebufferWidth,
							rfb.framebufferHeight,
							false);
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			} else {
				// Input enabled.
				synchronized (rfb) {
					try {
						rfb.writeKeyEvent(evt);
					} catch (Exception e) {
						e.printStackTrace();
					}
					rfb.notify();
				}
			}
		}
		// Don't ever pass keyboard events to AWT for default processing. 
		// Otherwise, pressing Tab would switch focus to ButtonPanel etc.
		evt.consume();
	}

	public void processLocalMouseWheelEvent(MouseWheelEvent evt) {
		if (viewer.rfb != null && rfb.inNormalProtocol) {
			synchronized(rfb) {
				try {
					rfb.writeWheelEvent(evt);
				} catch (Exception e) {
					e.printStackTrace();
				}
				rfb.notify();
			}
		}
	}

	public void processLocalMouseEvent(MouseEvent evt, boolean moved) {
		if (viewer.rfb != null && rfb.inNormalProtocol) {
			if (moved) {
				softCursorMove(evt.getX(), evt.getY());
			}
			synchronized (rfb) {
				try {
					rfb.writePointerEvent(evt);
				} catch (Exception e) {
					e.printStackTrace();
				}
				rfb.notify();
			}
		}
	}

	//
	// Ignored events.
	//

	public void mouseClicked(MouseEvent evt) {
	}
	public void mouseEntered(MouseEvent evt) {
	}
	public void mouseExited(MouseEvent evt) {
	}

	//////////////////////////////////////////////////////////////////
	//
	// Handle cursor shape updates (XCursor and RichCursor encodings).
	//

	boolean showSoftCursor = false;

	int[] softCursorPixels;
	MemoryImageSource softCursorSource;
	Image softCursor;

	int cursorX = 0, cursorY = 0;
	int cursorWidth, cursorHeight;
	int hotX, hotY;

	//
	// Handle cursor shape update (XCursor and RichCursor encodings).
	//

	synchronized void handleCursorShapeUpdate(
		int encodingType,
		int xhot,
		int yhot,
		int width,
		int height)
		throws IOException {

		int bytesPerRow = (width + 7) / 8;
		int bytesMaskData = bytesPerRow * height;

		softCursorFree();

		if (width * height == 0)
			return;

		// Ignore cursor shape data if requested by user.

		if (viewer.options.ignoreCursorUpdates) {
			if (encodingType == rfb.EncodingXCursor) {
				rfb.is.skipBytes(6 + bytesMaskData * 2);
			} else {
				// rfb.EncodingRichCursor
				rfb.is.skipBytes(width * height + bytesMaskData);
			}
			return;
		}

		// Decode cursor pixel data.

		softCursorPixels = new int[width * height];

		if (encodingType == rfb.EncodingXCursor) {

			// Read foreground and background colors of the cursor.
			byte[] rgb = new byte[6];
			rfb.is.readFully(rgb);
			int[] colors =
				{
					(0xFF000000 | (rgb[3] & 0xFF)
						<< 16 | (rgb[4] & 0xFF)
						<< 8 | (rgb[5] & 0xFF)),
					(0xFF000000 | (rgb[0] & 0xFF)
						<< 16 | (rgb[1] & 0xFF)
						<< 8 | (rgb[2] & 0xFF))};

			// Read pixel and mask data.
			byte[] pixBuf = new byte[bytesMaskData];
			rfb.is.readFully(pixBuf);
			byte[] maskBuf = new byte[bytesMaskData];
			rfb.is.readFully(maskBuf);

			// Decode pixel data into softCursorPixels[].
			byte pixByte, maskByte;
			int x, y, n, result;
			int i = 0;
			for (y = 0; y < height; y++) {
				for (x = 0; x < width / 8; x++) {
					pixByte = pixBuf[y * bytesPerRow + x];
					maskByte = maskBuf[y * bytesPerRow + x];
					for (n = 7; n >= 0; n--) {
						if ((maskByte >> n & 1) != 0) {
							result = colors[pixByte >> n & 1];
						} else {
							result = 0; // Transparent pixel
						}
						softCursorPixels[i++] = result;
					}
				}
				for (n = 7; n >= 8 - width % 8; n--) {
					if ((maskBuf[y * bytesPerRow + x] >> n & 1) != 0) {
						result = colors[pixBuf[y * bytesPerRow + x] >> n & 1];
					} else {
						result = 0; // Transparent pixel
					}
					softCursorPixels[i++] = result;
				}
			}

		} else {
			// encodingType == rfb.EncodingRichCursor

			// Read pixel and mask data.
			byte[] pixBuf = new byte[width * height * bytesPixel];
			rfb.is.readFully(pixBuf);
			byte[] maskBuf = new byte[bytesMaskData];
			rfb.is.readFully(maskBuf);

			// Decode pixel data into softCursorPixels[].
			byte pixByte, maskByte;
			int x, y, n, result;
			int i = 0;
			for (y = 0; y < height; y++) {
				for (x = 0; x < width / 8; x++) {
					maskByte = maskBuf[y * bytesPerRow + x];
					for (n = 7; n >= 0; n--) {
						if ((maskByte >> n & 1) != 0) {
							if (bytesPixel == 1)
							{
								result = 0;
								// sf@2005
								switch (viewer.options.eightBitColors)
								{
									case 1:
										result = cm8_256c.getRGB(pixBuf[i]);
										break;
									case 2:
									case 4:
										result = cm8_64c.getRGB(pixBuf[i]);
										break;
									case 3:
									case 5:
										result = cm8_8c.getRGB(pixBuf[i]);
										break;
								}
							}
							else
							{
								result =
									0xFF000000 | (pixBuf[i * 4 + 2] & 0xFF)
										<< 16 | (pixBuf[i * 4 + 1] & 0xFF)
										<< 8 | (pixBuf[i * 4 + 0] & 0xFF);
							}
						} else {
							result = 0; // Transparent pixel
						}
						softCursorPixels[i++] = result;
					}
				}
				for (n = 7; n >= 8 - width % 8; n--) {
					if ((maskBuf[y * bytesPerRow + x] >> n & 1) != 0) {
						if (bytesPixel == 1)
						{
							result = 0;
//							 sf@2005
							switch (viewer.options.eightBitColors)
							{
								case 1:
									result = cm8_256c.getRGB(pixBuf[i]);
									break;
								case 2:
								case 4:
									result = cm8_64c.getRGB(pixBuf[i]);
									break;
								case 3:
								case 5:
									result = cm8_8c.getRGB(pixBuf[i]);
									break;
							}						}
						else
						{
							result =
								0xFF000000 | (pixBuf[i * 4 + 1] & 0xFF)
									<< 16 | (pixBuf[i * 4 + 2] & 0xFF)
									<< 8 | (pixBuf[i * 4 + 3] & 0xFF);
						}
					} else {
						result = 0; // Transparent pixel
					}
					softCursorPixels[i++] = result;
				}
			}

		}

		// Draw the cursor on an off-screen image.

		softCursorSource =
			new MemoryImageSource(width, height, softCursorPixels, 0, width);
		softCursor = createImage(softCursorSource);

		// Set remaining data associated with cursor.

		cursorWidth = width;
		cursorHeight = height;
		hotX = xhot;
		hotY = yhot;

		showSoftCursor = true;

		// Show the cursor.

		repaint(
			viewer.deferCursorUpdates,
			cursorX - hotX,
			cursorY - hotY,
			cursorWidth,
			cursorHeight);
	}

	//
	// marscha - PointerPos
	// Handle cursor position update (PointerPos encoding).
	//

	synchronized void handleCursorPosUpdate(
		int x,
		int y) {
		if (x >= rfb.framebufferWidth)
			x = rfb.framebufferWidth - 1;
		if (y >= rfb.framebufferHeight)
			y = rfb.framebufferHeight - 1;

		softCursorMove(x, y);
	}
	
	//
	// softCursorMove(). Moves soft cursor into a particular location.
	//

	synchronized void softCursorMove(int x, int y) {
		if (showSoftCursor) {
			repaint(
				viewer.deferCursorUpdates,
				cursorX - hotX,
				cursorY - hotY,
				cursorWidth,
				cursorHeight);
			repaint(
				viewer.deferCursorUpdates,
				x - hotX,
				y - hotY,
				cursorWidth,
				cursorHeight);
		}

		cursorX = x;
		cursorY = y;
	}

	//
	// softCursorFree(). Remove soft cursor, dispose resources.
	//

	synchronized void softCursorFree() {
		if (showSoftCursor) {
			showSoftCursor = false;
			softCursor = null;
			softCursorSource = null;
			softCursorPixels = null;

			repaint(
				viewer.deferCursorUpdates,
				cursorX - hotX,
				cursorY - hotY,
				cursorWidth,
				cursorHeight);
		}
	}
}
