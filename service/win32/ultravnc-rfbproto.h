#ifndef ULTRAVNC_RFB_PROTO_H
#define ULTRAVNC_RFB_PROTO_H

#define KEEPALIVE_HEADROOM 1
#define KEEPALIVE_INTERVAL 5
#define FT_RECV_TIMEOUT    30

// adzm 2010-08
#define SOCKET_KEEPALIVE_TIMEOUT 10000
#define SOCKET_KEEPALIVE_INTERVAL 1000

#define rfbEncodingUltra2   10

// adzm - 2010-07 - Extended clipboard support
// this struct is used as the data within an rfbServerCutTextMsg or rfbClientCutTextMsg.
typedef struct {
	uint32_t flags;		// see rfbExtendedClipboardDataFlags

	// followed by unsigned char data[(rfbServerCutTextMsg|rfbClientCutTextMsg).length - sz_rfbExtendedClipboardData]
} rfbExtendedClipboardData;

#define sz_rfbExtendedClipboardData 4

typedef enum {
	// formats
	clipText		= 0x00000001,	// Unicode text (UTF-8 encoding)
	clipRTF			= 0x00000002,	// Microsoft RTF format
	clipHTML		= 0x00000004,	// Microsoft HTML clipboard format
	clipDIB			= 0x00000008,	// Microsoft DIBv5
	// line endings are not touched and remain as \r\n for Windows machines. Terminating NULL characters are preserved.

	// Complex formats
	// These formats should also have 3 more CARD32 values after rfbExtendedClipboardData.flags. This will allow them
	// to set up more complex messages (such as preview) or subformats (such as lossless, png, jpeg, lossy) etc.
	// The rest should follow the standard format of a 32-bit length of the uncompressed data, followed by the data.
	//
	// Please note none of these are implemented yet, but seem obvious enough that their values are reserved here
	// for posterity.
	clipFiles		= 0x00000010,	// probably also more than one file
	clipFormatMask	= 0x0000FFFF,

	// reserved
	clipReservedMask= 0x00FF0000,	// more than likely will be used for more formats, but may be used for more actions
									// or remain unused for years to come.

	// actions
	clipCaps		= 0x01000000,	// which formats are supported / desired.
									// Message data should include limits on maximum automatic uncompressed data size for each format
									// in 32-bit values (in order of enum value). If the data exceeds that value, it must be requested.
									// This can be used to disable the clipboard entirely by setting no supported formats, or to
									// only enable manual clipboard transfers by setting the maximum sizes to 0.
									// can be combined with other actions to denote actions that are supported
									// The server must send this to the client to notify that it understands the new clipboard format.
									// The client may respond with its own clipCaps; otherwise the server should use the defaults.
									// Currently, the defaults are the messages and formats defined in this initial implementation
									// that are common to both server and viewer:
									//    clipCaps | clipRequest | clipProvide | (clipNotify if viewer, clipPeek if server)
									//    clipText | clipRTF | clipHTML | clipDIB
									//    (Note that clipNotify is only relevant from server->viewer, and clipPeek is only relevant
									//     from viewer->server. Therefore they are left out of the defaults but can be set with the
									//     rest of the caps if desired.)
									// It is also strongly recommended to set up maximum sizes for the formats since currently
									// the data is sent synchronously and cannot be interrupted. If data exceeds the maximum size,
									// then the server should send the clipNotify so the client may send clipRequest. Current default 
									// limits were somewhat arbitrarily chosen as 2mb (10mb for text) and 0 for image
									// Note that these limits are referring to the length of uncompressed data.
	clipRequest		= 0x02000000,	// request clipboard data (should be combined with desired formats)
									// Message should be empty
									// Response should be a clipProvide message with appropriate formats. This should ignore any
									// maximum size limitations specified in clipCaps.
	clipPeek		= 0x04000000,	// Peek at what is currently available in the clipboard.
									// Message should be empty
									// Respond with clipNotify including all available formats in the flags
	clipNotify		= 0x08000000,	// notify that the formats combined with the flags are available for transfer.
									// Message should be empty
									// When a clipProvide message is received, then all formats notified as being available are 
									// invalidated. Therefore, when implementing, ensure that clipProvide messages are sent before
									// clipNotify messages, specifically when in response to a change in the clipboard
	clipProvide		= 0x10000000,	// send clipboard data (should be combined with sent formats)
									// All message data including the length is compressed by a single zlib stream.
									// First is the 32-bit length of the uncompressed data, followed by the data itself
									// Repeat for each format listed in order of enum value
									// Invalidate any formats that were notified as being available.
	clipActionMask	= 0xFF000000,

	clipInvalid		= 0xFFFFFFFF,

} rfbExtendedClipboardDataFlags;



#endif

