
//  Copyright (C) 2002-2005 Ultr@VNC Team.  All Rights Reserved.
//  Copyright (C) 2004 Kenn Min Chong, John Witchel.  All Rights Reserved.
//  Copyright (C) 2004 Alban Chazot.  All Rights Reserved.
//  Copyright (C) 2001,2002 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2002 Constantin Kaplinsky.  All Rights Reserved.
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

//
// VncViewer.java - the VNC viewer applet.  This class mainly just sets up the
// user interface, leaving it to the VncCanvas to do the actual rendering of
// a VNC desktop.
//

// Alban Chazot - Carmi Grenoble July 5th 2004 
// * Add support for Ultr@VNC mslogon feature.
//   You can now be connected to a Ultr@VNC box with mslogon required.
//   Thanks to Wim Vandersmissen, who provide a TightVNC viewer patch do to so.
//   That give me the idea to provide it in the java viewer too.
//
// * Add ScrollPanel to applet mode
//
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;
import javax.swing.*;

public class VncViewer extends java.applet.Applet
  implements java.lang.Runnable, WindowListener {

  boolean inAnApplet = true;
  boolean inSeparateFrame = false;

  // mslogon support

  boolean mslogon = false;

  // mslogon support end

  //
  // main() is called when run as a java program from the command line.
  // It simply runs the applet inside a newly-created frame.
  //

  public static void main(String[] argv) {
    VncViewer v = new VncViewer();
    v.mainArgs = argv;
    v.inAnApplet = false;
    v.inSeparateFrame = true;

    v.init();
    v.start();
  }

  String[] mainArgs;

  RfbProto rfb;
  Thread rfbThread;

  Frame vncFrame;
  Container vncContainer;
  ScrollPane desktopScrollPane;
  GridBagLayout gridbag;
  ButtonPanel buttonPanel;
  AuthPanel authenticator;
  VncCanvas vc;
  OptionsFrame options;
  ClipboardFrame clipboard;
  RecordingFrame rec;
  FTPFrame ftp; // KMC: FTP Frame declaration

  // Control session recording.
  Object recordingSync;
  String sessionFileName;
  boolean recordingActive;
  boolean recordingStatusChanged;
  String cursorUpdatesDef;
  String eightBitColorsDef;

  // Variables read from parameter values.
  String host;
  int port;
  String passwordParam;
  String encPasswordParam;
  boolean showControls;
  boolean showOfflineDesktop;
  int deferScreenUpdates;
  int deferCursorUpdates;
  int deferUpdateRequests;

  // mslogon support 2
  String usernameParam;
  String encUsernameParam;
  String dm; 
  byte[] domain = new byte[256];
  byte[] user = new byte[256];
  byte[] passwd = new byte[32];
  int i;
  // mslogon support 2 end

  //
  // init()
  //

  public void init() {

    readParameters();

    if (inSeparateFrame) {
      vncFrame = new Frame("Ultr@VNC");
      if (!inAnApplet) {
	vncFrame.add("Center", this);
      }
      vncContainer = vncFrame;
    } else {
      vncContainer = this;
    }

    recordingSync = new Object();

    options = new OptionsFrame(this);
    clipboard = new ClipboardFrame(this);
    // authenticator = new AuthPanel(false);   // mslogon support : go to connectAndAuthenticate()
    if (RecordingFrame.checkSecurity())
      rec = new RecordingFrame(this);

    sessionFileName = null;
    recordingActive = false;
    recordingStatusChanged = false;
    cursorUpdatesDef = null;
    eightBitColorsDef = null;

    if (inSeparateFrame)
      vncFrame.addWindowListener(this);

    ftp = new FTPFrame(this); // KMC: FTPFrame creation
    rfbThread = new Thread(this);
    rfbThread.start();
  }

  public void update(Graphics g) {
  }

  //
  // run() - executed by the rfbThread to deal with the RFB socket.
  //

  public void run() {

    gridbag = new GridBagLayout();
    vncContainer.setLayout(gridbag);

    GridBagConstraints gbc = new GridBagConstraints();
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.anchor = GridBagConstraints.NORTHWEST;

    if (showControls) {
      buttonPanel = new ButtonPanel(this);
      gridbag.setConstraints(buttonPanel, gbc);
      vncContainer.add(buttonPanel);
    }

    try {
      connectAndAuthenticate();

      doProtocolInitialisation();

      vc = new VncCanvas(this);
      gbc.weightx = 1.0;
      gbc.weighty = 1.0;

	// Add ScrollPanel to applet mode

	// Create a panel which itself is resizeable and can hold
	// non-resizeable VncCanvas component at the top left corner.
	Panel canvasPanel = new Panel();
	canvasPanel.setLayout(new FlowLayout(FlowLayout.LEFT, 0, 0));
	canvasPanel.add(vc);

	// Create a ScrollPane which will hold a panel with VncCanvas
	// inside.
	desktopScrollPane = new ScrollPane(ScrollPane.SCROLLBARS_AS_NEEDED);
	gbc.fill = GridBagConstraints.BOTH;
	gridbag.setConstraints(desktopScrollPane, gbc);
	desktopScrollPane.add(canvasPanel);

      if (inSeparateFrame) {
	// Finally, add our ScrollPane to the Frame window.
	vncFrame.add(desktopScrollPane);
	vncFrame.setTitle(rfb.desktopName);
	vncFrame.pack();
	vc.resizeDesktopFrame();
      } else {
	// Finally, add the scrollable panel component to the Applet.
        gridbag.setConstraints(desktopScrollPane, gbc);
	add(desktopScrollPane);

	// Add ScrollPanel to applet mode end
 
	validate();

      }

      if (showControls)
	buttonPanel.enableButtons();

      moveFocusToDesktop();
      vc.processNormalProtocol();

    } catch (NoRouteToHostException e) {
      e.printStackTrace();
      fatalError("Network error: no route to server: " + host);
    } catch (UnknownHostException e) {
      e.printStackTrace();
      fatalError("Network error: server name unknown: " + host);
    } catch (ConnectException e) {
      e.printStackTrace();
      fatalError("Network error: could not connect to server: " +
		 host + ":" + port);
    } catch (EOFException e) {
      e.printStackTrace();
      if (showOfflineDesktop) {
	System.out.println("Network error: remote side closed connection");
	if (vc != null) {
	  vc.enableInput(false);
	}
	if (inSeparateFrame) {
	  vncFrame.setTitle(rfb.desktopName + " [disconnected]");
	}
	if (rfb != null) {
	  rfb.close();
	  rfb = null;
	}
	if (showControls && buttonPanel != null) {
	  buttonPanel.disableButtonsOnDisconnect();
	  if (inSeparateFrame) {
	    vncFrame.pack();
	  } else {
	    validate();
	  }
	}
      } else {
	fatalError("Network error: remote side closed connection");
      }
    } catch (IOException e) {
      String str = e.getMessage();
      e.printStackTrace();
      if (str != null && str.length() != 0) {
	fatalError("Network Error: " + str);
      } else {
	fatalError(e.toString());
      }
    } catch (Exception e) {
      String str = e.getMessage();
      e.printStackTrace();
      if (str != null && str.length() != 0) {
	fatalError("Error: " + str);
      } else {
	fatalError(e.toString());
      }
    }
    
  }


  //
  // Connect to the RFB server and authenticate the user.
  //

  void connectAndAuthenticate() throws Exception {

    // If "ENCPASSWORD" parameter is set, decrypt the password into
    // the passwordParam string.

    if (encPasswordParam != null) {
      // ENCPASSWORD is hexascii-encoded. Decode.
      byte[] pw = {0, 0, 0, 0, 0, 0, 0, 0};
      int len = encPasswordParam.length() / 2;
      if (len > 8)
	len = 8;
      for (int i = 0; i < len; i++) {
	String hex = encPasswordParam.substring(i*2, i*2+2);
	Integer x = new Integer(Integer.parseInt(hex, 16));
	pw[i] = x.byteValue();
      }

      // Decrypt the password.
      byte[] key = {23, 82, 107, 6, 35, 78, 88, 7};
      DesCipher des = new DesCipher(key);
      des.decrypt(pw, 0, pw, 0);
      passwordParam = new String(pw);
    }

    // If a password parameter ("PASSWORD" or "ENCPASSWORD") is set,
    // don't ask user a password, get one from passwordParam instead.
    // Authentication failures would be fatal.

    if (passwordParam != null) {
      if (inSeparateFrame) {
	vncFrame.pack();
	vncFrame.show();
      } else {
	validate();
      }
      if (!tryAuthenticate(usernameParam,passwordParam)) {
	throw new Exception("VNC authentication failed");
      }
      return;
    }

    // There is no "PASSWORD" or "ENCPASSWORD" parameters -- ask user
    // for a password, try to authenticate, retry on authentication
    // failures.

    // mslogon support
    //
    // Detect Auth Protocol (Ultr@VNC or the standard One)
    // To know if we must show the username box
    //

    
    prologueDetectAuthProtocol() ;

    authenticator = new AuthPanel(mslogon);
    
    // mslogon support end
    
    GridBagConstraints gbc = new GridBagConstraints();
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.anchor = GridBagConstraints.NORTHWEST;
    gbc.weightx = 1.0;
    gbc.weighty = 1.0;
    gbc.ipadx = 100;
    gbc.ipady = 50;
    
    gridbag.setConstraints(authenticator, gbc);
    vncContainer.add(authenticator);

    if (inSeparateFrame) {
      vncFrame.pack();
      vncFrame.show();
    } else {
      validate();
      // FIXME: here moveFocusToPasswordField() does not always work
      // under Netscape 4.7x/Java 1.1.5/Linux. It seems like this call
      // is being executed before the password field of the
      // authenticator is fully drawn and activated, therefore
      // requestFocus() does not work. Currently, I don't know how to
      // solve this problem.
      //   -- const
      
     //mslogon support 
      authenticator.moveFocusToUsernameField();
     //mslogon support end
    }

    while (true) {
      // Wait for user entering a password, or a username and a password
      synchronized(authenticator) {
	try {
	  authenticator.wait();
	} catch (InterruptedException e) {
	}
      }

      // Try to authenticate with a given password.
      //mslogon support 
      String us;
      if (mslogon) { us = authenticator.username.getText(); }
      else { us = "";}

      if (tryAuthenticate(us,authenticator.password.getText()))
	break;
      //mslogon support end

      // Retry on authentication failure.
      authenticator.retry();
    }

    vncContainer.remove(authenticator);
  }

  // mslogon support
  //
  // Detect Server rfb Protocol to know the auth Method
  // Perform a connexion to detect the Serverminor 
  //

  void prologueDetectAuthProtocol() throws Exception {

    rfb = new RfbProto(host, port, this, null, 0); // Modif: troessner - sf@2007: not yet used

    rfb.readVersionMsg();

    System.out.println("RFB server supports protocol version " +
		       rfb.serverMajor + "." + rfb.serverMinor);

    // Mslogon support 
    if (rfb.serverMinor == 4) {
	    mslogon = true;    
	    System.out.println("Ultr@VNC mslogon detected");
    }
    
    rfb.writeVersionMsg();

  }
  
  // mslogon support end


  //
  // Try to authenticate with a given password.
  //

  boolean tryAuthenticate(String us, String pw) throws Exception {
    
    rfb = new RfbProto(host, port, this, null, 0); // Modif: troessner - sf@2007: not yet used

    rfb.readVersionMsg();

    System.out.println("RFB server supports protocol version " +
		       rfb.serverMajor + "." + rfb.serverMinor);

    rfb.writeVersionMsg();

    int authScheme = rfb.readAuthScheme();

    switch (authScheme) {

    case RfbProto.NoAuth:
      System.out.println("No authentication needed");
      return true;

    case RfbProto.VncAuth:
      
      if (mslogon) {
    	  System.out.println("showing JOptionPane warning.");
    	  int n = JOptionPane.showConfirmDialog(
                  vncFrame, "The current authentication method does not transfer your password securely."
                   + "Do you want to continue?",
                  "Warning",
                  JOptionPane.YES_NO_OPTION);
          if (n != JOptionPane.YES_OPTION) {
              throw new Exception("User cancelled insecure MS-Logon");
          }
      }
      // mslogon support 
      byte[] challengems = new byte[64];
      if (mslogon) { 
        // copy the us (user) parameter into the user Byte formated variable 
	System.arraycopy(us.getBytes(), 0, user, 0, us.length() );
	// and pad it with Null
	if (us.length() < 256) {
	  for (i=us.length(); i<256; i++){ user[i]= 0; }
	}

	dm = ".";
        // copy the dm (domain) parameter into the domain Byte formated variable 
        System.arraycopy(dm.getBytes(), 0, domain, 0, dm.length() );
	// and pad it with Null
	if (dm.length() < 256) {
	  for (i=dm.length(); i<256; i++){ domain[i]= 0; }
	}
		
        // equivalent of vncEncryptPasswdMS
	 
        // copy the pw (password) parameter into the password Byte formated variable 
        System.arraycopy(pw.getBytes(), 0, passwd, 0, pw.length() );
	// and pad it with Null
	if (pw.length() < 32) {
	  for (i=pw.length(); i<32; i++){ passwd[i]= 0; }
	}

	// Encrypt the full given password
        byte[] fixedkey = {23, 82, 107, 6, 35, 78, 88, 7};
        DesCipher desme = new DesCipher(fixedkey);
        desme.encrypt(passwd, 0, passwd, 0);

        // end equivalent of vncEncryptPasswdMS

	// get the mslogon Challenge from server 
        rfb.is.readFully(challengems);
      }
      // mslogon support end
      
      byte[] challenge = new byte[16];
      rfb.is.readFully(challenge);

      if (pw.length() > 8)
	pw = pw.substring(0, 8); // Truncate to 8 chars

      // vncEncryptBytes in the UNIX libvncauth truncates password
      // after the first zero byte. We do to.
      int firstZero = pw.indexOf(0);
      if (firstZero != -1)
	pw = pw.substring(0, firstZero);

      // mslogon support
      if (mslogon ){ 
        for (i= 0; i<32; i++) {
		challengems[i]= (byte) (passwd[i]^challengems[i]);
        }
        rfb.os.write(user);
        rfb.os.write(domain);
        rfb.os.write(challengems);
      }
      // mslogon support end

      byte[] key = {0, 0, 0, 0, 0, 0, 0, 0};
      System.arraycopy(pw.getBytes(), 0, key, 0, pw.length());

      DesCipher des = new DesCipher(key);

      des.encrypt(challenge, 0, challenge, 0);
      des.encrypt(challenge, 8, challenge, 8);
     
      rfb.os.write(challenge);

      int authResult = rfb.is.readInt();

      switch (authResult) {
      case RfbProto.VncAuthOK:
	System.out.println("VNC authentication succeeded");
	return true;
      case RfbProto.VncAuthFailed:
	System.out.println("VNC authentication failed");
	break;
      case RfbProto.VncAuthTooMany:
	throw new Exception("VNC authentication failed - too many tries");
      default:
	throw new Exception("Unknown VNC authentication result " + authResult);
      }
      break;

    case RfbProto.MsLogon:
    	System.out.println("MS-Logon (DH) detected");
		if (AuthMsLogon(us, pw)) {
			return true;
		}
		break;
    default:
      throw new Exception("Unknown VNC authentication scheme " + authScheme);
    }
    return false;
  }

  // marscha@2006: Try to better hide the windows password.
  // I know that this is no breakthrough in modern cryptography.
  // It's just a patch/kludge/workaround.
  boolean AuthMsLogon(String us, String pw) throws Exception {
	byte user[] = new byte[256];
	byte passwd[] = new byte[64];

    long gen  = rfb.is.readLong();
	long mod  = rfb.is.readLong();
	long resp = rfb.is.readLong();
		
	DH dh = new DH(gen, mod);
	long pub = dh.createInterKey();

	rfb.os.write(DH.longToBytes(pub));

	long key = dh.createEncryptionKey(resp);
	System.out.println("gen=" + gen + ", mod=" + mod
			+ ", pub=" + pub + ", key=" + key);

	DesCipher des = new DesCipher(DH.longToBytes(key));

	System.arraycopy(us.getBytes(), 0, user, 0, us.length() );
	// and pad it with Null
	if (us.length() < 256) {
	  for (i=us.length(); i<256; i++){ user[i]= 0; }
	}

    // copy the pw (password) parameter into the password Byte formated variable 
    System.arraycopy(pw.getBytes(), 0, passwd, 0, pw.length() );
	// and pad it with Null
	if (pw.length() < 32) {
	  for (i=pw.length(); i<32; i++){ passwd[i]= 0; }
	}

	//user = domain + "\\" + user;

	des.encryptText(user, user, DH.longToBytes(key));
	des.encryptText(passwd, passwd, DH.longToBytes(key));
	
	rfb.os.write(user);
	rfb.os.write(passwd);


    int authResult = rfb.is.readInt();

    switch (authResult) {
      case RfbProto.VncAuthOK:
	System.out.println("MS-Logon (DH) authentication succeeded");
	return true;
      case RfbProto.VncAuthFailed:
	System.out.println("MS-Logon (DH) authentication failed");
	break;
      case RfbProto.VncAuthTooMany:
	throw new Exception("MS-Logon (DH) authentication failed - too many tries");
      default:
	throw new Exception("Unknown MS-Logon (DH) authentication result " + authResult);
    }
    return false;
}


  //
  // Do the rest of the protocol initialisation.
  //

  void doProtocolInitialisation() throws IOException {

    rfb.writeClientInit();

    rfb.readServerInit();

    System.out.println("Desktop name is " + rfb.desktopName);
    System.out.println("Desktop size is " + rfb.framebufferWidth + " x " +
		       rfb.framebufferHeight);

    setEncodings();
  }


  //
  // Send current encoding list to the RFB server.
  //

  void setEncodings() {
    try {
      if (rfb != null && rfb.inNormalProtocol) {
	rfb.writeSetEncodings(options.encodings, options.nEncodings);
	if (vc != null) {
	  vc.softCursorFree();
	}
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }


  //
  // setCutText() - send the given cut text to the RFB server.
  //

  void setCutText(String text) {
    try {
      if (rfb != null && rfb.inNormalProtocol) {
	rfb.writeClientCutText(text);
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }


  //
  // Order change in session recording status. To stop recording, pass
  // null in place of the fname argument.
  //

  void setRecordingStatus(String fname) {
    synchronized(recordingSync) {
      sessionFileName = fname;
      recordingStatusChanged = true;
    }
  }

  //
  // Start or stop session recording. Returns true if this method call
  // causes recording of a new session.
  //

  boolean checkRecordingStatus() throws IOException {
    synchronized(recordingSync) {
      if (recordingStatusChanged) {
	recordingStatusChanged = false;
	if (sessionFileName != null) {
	  startRecording();
	  return true;
	} else {
	  stopRecording();
	}
      }
    }
    return false;
  }

  //
  // Start session recording.
  //

  protected void startRecording() throws IOException {
    synchronized(recordingSync) {
      if (!recordingActive) {
	// Save settings to restore them after recording the session.
	cursorUpdatesDef =
	  options.choices[options.cursorUpdatesIndex].getSelectedItem();
	eightBitColorsDef =
	  options.choices[options.eightBitColorsIndex].getSelectedItem();
	// Set options to values suitable for recording.
	options.choices[options.cursorUpdatesIndex].select("Disable");
	options.choices[options.cursorUpdatesIndex].setEnabled(false);
	options.setEncodings();
	options.choices[options.eightBitColorsIndex].select("Full");
	options.choices[options.eightBitColorsIndex].setEnabled(false);
	options.setColorFormat();
      } else {
	rfb.closeSession();
      }

      System.out.println("Recording the session in " + sessionFileName);
      rfb.startSession(sessionFileName);
      recordingActive = true;
    }
  }

  //
  // Stop session recording.
  //

  protected void stopRecording() throws IOException {
    synchronized(recordingSync) {
      if (recordingActive) {
	// Restore options.
	options.choices[options.cursorUpdatesIndex].select(cursorUpdatesDef);
	options.choices[options.cursorUpdatesIndex].setEnabled(true);
	options.setEncodings();
	options.choices[options.eightBitColorsIndex].select(eightBitColorsDef);
	options.choices[options.eightBitColorsIndex].setEnabled(true);
	options.setColorFormat();

	rfb.closeSession();
	System.out.println("Session recording stopped.");
      }
      sessionFileName = null;
      recordingActive = false;
    }
  }


  //
  // readParameters() - read parameters from the html source or from the
  // command line.  On the command line, the arguments are just a sequence of
  // param_name/param_value pairs where the names and values correspond to
  // those expected in the html applet tag source.
  //

  public void readParameters() {
    host = readParameter("HOST", !inAnApplet);
    if (host == null) {
      host = getCodeBase().getHost();
      if (host.equals("")) {
	fatalError("HOST parameter not specified");
      }
    }

    String str = readParameter("PORT", true);
    port = Integer.parseInt(str);

    if (inAnApplet) {
      str = readParameter("Open New Window", false);
      if (str != null && str.equalsIgnoreCase("Yes"))
	inSeparateFrame = true;
    }

    encPasswordParam = readParameter("ENCPASSWORD", false);
    if (encPasswordParam == null)
      passwordParam = readParameter("PASSWORD", false);

    // "Show Controls" set to "No" disables button panel.
    showControls = true;
    str = readParameter("Show Controls", false);
    if (str != null && str.equalsIgnoreCase("No"))
      showControls = false;

    // Do we continue showing desktop on remote disconnect?
    showOfflineDesktop = false;
    str = readParameter("Show Offline Desktop", false);
    if (str != null && str.equalsIgnoreCase("Yes"))
      showOfflineDesktop = true;

    // Fine tuning options.
    deferScreenUpdates = readIntParameter("Defer screen updates", 20);
    deferCursorUpdates = readIntParameter("Defer cursor updates", 10);
    deferUpdateRequests = readIntParameter("Defer update requests", 50);
  }

  public String readParameter(String name, boolean required) {
    if (inAnApplet) {
      String s = getParameter(name);
      if ((s == null) && required) {
	fatalError(name + " parameter not specified");
      }
      return s;
    }

    for (int i = 0; i < mainArgs.length; i += 2) {
      if (mainArgs[i].equalsIgnoreCase(name)) {
	try {
	  return mainArgs[i+1];
	} catch (Exception e) {
	  if (required) {
	    fatalError(name + " parameter not specified");
	  }
	  return null;
	}
      }
    }
    if (required) {
      fatalError(name + " parameter not specified");
    }
    return null;
  }

  int readIntParameter(String name, int defaultValue) {
    String str = readParameter(name, false);
    int result = defaultValue;
    if (str != null) {
      try {
	result = Integer.parseInt(str);
      } catch (NumberFormatException e) { }
    }
    return result;
  }

  //
  // moveFocusToDesktop() - move keyboard focus either to the
  // VncCanvas or to the AuthPanel.
  //

  void moveFocusToDesktop() {
    if (vncContainer != null) {
      if (vc != null && vncContainer.isAncestorOf(vc)) {
	vc.requestFocus();
      } else if (vncContainer.isAncestorOf(authenticator)) {
	authenticator.moveFocusToPasswordField();
      }
    }
  }

  //
  // disconnect() - close connection to server.
  //

  boolean disconnectRequested = false;

  synchronized public void disconnect() {
    disconnectRequested = true;
    if (rfb != null) {
      rfb.close();
      rfb = null;
    }
    System.out.println("Disconnect");
    options.dispose();
    clipboard.dispose();
    if (rec != null)
      rec.dispose();

    if (inAnApplet) {
      vncContainer.removeAll();
      Label errLabel = new Label("Disconnected");
      errLabel.setFont(new Font("Helvetica", Font.PLAIN, 12));
      vncContainer.setLayout(new FlowLayout(FlowLayout.LEFT, 30, 30));
      vncContainer.add(errLabel);
      if (inSeparateFrame) {
	vncFrame.pack();
      } else {
	validate();
      }
      rfbThread.stop();
    } else {
      System.exit(0);
    }
  }

  //
  // fatalError() - print out a fatal error message.
  //

  synchronized public void fatalError(String str) {
    if (rfb != null) {
      rfb.close();
      rfb = null;
    }
    System.out.println(str);

    if (disconnectRequested) {
      // Not necessary to show error message if the error was caused
      // by I/O problems after the disconnect() method call.
      disconnectRequested = false;
      return;
    }

    if (inAnApplet) {
      vncContainer.removeAll();
      Label errLabel = new Label(str);
      errLabel.setFont(new Font("Helvetica", Font.PLAIN, 12));
      vncContainer.setLayout(new FlowLayout(FlowLayout.LEFT, 30, 30));
      vncContainer.add(errLabel);
      if (inSeparateFrame) {
	vncFrame.pack();
      } else {
	validate();
      }
      Thread.currentThread().stop();
    } else {
      System.exit(1);
    }
  }


  //
  // This method is called before the applet is destroyed.
  //

  public void destroy() {
    vncContainer.removeAll();
    options.dispose();
    clipboard.dispose();
    if (ftp != null)
      ftp.dispose();
    if (rec != null)
      rec.dispose();
    if (rfb != null)
      rfb.close();
    if (inSeparateFrame)
      vncFrame.dispose();
  }


  //
  // Close application properly on window close event.
  //

  public void windowClosing(WindowEvent evt) {
    if (rfb != null)
      disconnect();

    vncFrame.dispose();
    if (!inAnApplet) {
      System.exit(0);
    }
  }

  //
  // Move the keyboard focus to the password field on window activation.
  //

  public void windowActivated(WindowEvent evt) {
    if (vncFrame.isAncestorOf(authenticator))
      authenticator.moveFocusToPasswordField();
  }

  //
  // Ignore window events we're not interested in.
  //

  public void windowDeactivated (WindowEvent evt) {}
  public void windowOpened(WindowEvent evt) {}
  public void windowClosed(WindowEvent evt) {}
  public void windowIconified(WindowEvent evt) {}
  public void windowDeiconified(WindowEvent evt) {}
}
