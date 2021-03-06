/*
 *
 * Includes for ch34x.c
 *
 */

/*
 * Baud rate and default timeout
 */
#define DEFAULT_BAUD_RATE 9600
#define DEFAULT_TIMEOUT   2000

/*
 * CMSPAR, some architectures can't have space and mark parity.
 */

#ifndef CMSPAR
#define CMSPAR			0
#endif

/*
 * Major and minor numbers.
 */

#define CH34X_TTY_MAJOR		169
#define CH34X_TTY_MINORS	256

/*
 * Requests.
 */

#define USB_RT_CH34X		(USB_TYPE_CLASS | USB_RECIP_INTERFACE)

//Vendor define
#define VENDOR_WRITE_TYPE		0x40
#define VENDOR_READ_TYPE		0xC0

#define VENDOR_READ				0x95
#define VENDOR_WRITE			0x9A
#define VENDOR_SERIAL_INIT		0xA1
#define VENDOR_MODEM_OUT		0xA4
#define VENDOR_VERSION			0x5F

/*
 * Output control lines.
 */
#define CH34X_CTRL_OUT		0x10
#define CH34X_CTRL_DTR		0x20
#define	CH34X_CTRL_RTS		0x40

/******************************/
/* interrupt pipe definitions */
/******************************/
/* always 4 interrupt bytes */
/* first irq byte indicates uart status */
/* third irq byte indicates modem status */

/* second interrupt byte */
#define CH34X_MULT_STAT 0x04 /* multiple status since last interrupt event */

/*
 * status returned in third interrupt answer byte,
 * inverted in data from irq
 */
#define CH34X_CTRL_CTS		0x01
#define CH34X_CTRL_DSR		0x02
#define CH34X_CTRL_RI		0x04
#define CH34X_CTRL_DCD		0x08
#define CH34X_MODEM_STAT 	0x0f

/*
 * Input line errors.
 */
#define CH34X_CTRL_TYPE_MODEM	BIT(3)
#define CH34X_CTRL_TYPE_FRAMING BIT(2) | BIT(6)
#define CH34X_CTRL_TYPE_PARITY  BIT(2)
#define CH34X_CTRL_TYPE_OVERRUN BIT(1)

/*
 * LCR defines.
 */
#define CH34X_LCR_ENABLE_RX    0x80
#define CH34X_LCR_ENABLE_TX    0x40
#define CH34X_LCR_PAR_SPACE    0x38
#define CH34X_LCR_PAR_MARK     0x28
#define CH34X_LCR_PAR_EVEN     0x18
#define CH34X_LCR_PAR_ODD      0x08
#define CH34X_LCR_STOP_BITS_2  0x04
#define CH34X_LCR_CS8          0x03
#define CH34X_LCR_CS7          0x02
#define CH34X_LCR_CS6          0x01
#define CH34X_LCR_CS5          0x00

#define CH34X_REG_BREAK        0x05
#define CH34X_REG_LCR          0x18
#define CH34X_NBREAK_BITS      0x01

/*
 * Internal driver structures.
 */

/*
 * The only reason to have several buffers is to accommodate assumptions
 * in line disciplines. They ask for empty space amount, receive our URB size,
 * and proceed to issue several 1-character writes, assuming they will fit.
 * The very first write takes a complete URB. Fortunately, this only happens
 * when processing onlcr, so we only need 2 buffers. These values must be
 * powers of 2.
 */
#define CH34X_NW  16
#define CH34X_NR  16

struct ch34x_wb {
	unsigned char *buf;
	dma_addr_t dmah;
	int len;
	int use;
	struct urb		*urb;
	struct ch34x		*instance;
};

struct ch34x_rb {
	int			size;
	unsigned char		*base;
	dma_addr_t		dma;
	int			index;
	struct ch34x		*instance;
};

struct usb_ch34x_line_coding {
	__le32	dwDTERate;
	__u8	bCharFormat;
#define USB_CH34X_1_STOP_BITS			0
#define USB_CH34X_1_5_STOP_BITS			1
#define USB_CH34X_2_STOP_BITS			2

	__u8	bParityType;
#define USB_CH34X_NO_PARITY				0
#define USB_CH34X_ODD_PARITY			1
#define USB_CH34X_EVEN_PARITY			2
#define USB_CH34X_MARK_PARITY			3
#define USB_CH34X_SPACE_PARITY			4

	__u8	bDataBits;
} __attribute__ ((packed));

struct ch34x {
	struct usb_device *dev;				/* the corresponding usb device */
	struct usb_interface *data;			/* data interface */
	struct tty_port port;			 	/* our tty port data */
	struct urb *ctrlurb;				/* urbs */
	u8 *ctrl_buffer;				/* buffers of urbs */
	dma_addr_t ctrl_dma;				/* dma handles of buffers */
	struct ch34x_wb wb[CH34X_NW];
	unsigned long read_urbs_free;
	struct urb *read_urbs[CH34X_NR];
	struct ch34x_rb read_buffers[CH34X_NR];
	int rx_buflimit;
	int rx_endpoint;
	spinlock_t read_lock;
	int write_used;					/* number of non-empty write buffers */
	int transmitting;
	spinlock_t write_lock;
	struct mutex mutex;
	bool disconnected;
	struct usb_ch34x_line_coding line;		/* bits, stop, parity */
	struct work_struct work;			/* work queue entry for line discipline waking up */
	unsigned int ctrlin;				/* input control lines (DCD, DSR, RI, break, overruns) */
	unsigned int ctrlout;				/* output control lines (DTR, RTS) */
	struct async_icount iocount;			/* counters for control line changes */
	struct async_icount oldcount;			/* for comparison of counter */
	wait_queue_head_t wioctl;			/* for ioctl */
	unsigned int writesize;				/* max packet size for the output bulk endpoint */
	unsigned int readsize,ctrlsize;			/* buffer sizes for freeing */
	unsigned int minor;				/* ch34x minor number */
	unsigned char clocal;				/* termios CLOCAL */
	unsigned int susp_count;			/* number of suspended interfaces */
	u8 bInterval;
	struct usb_anchor delayed;			/* writes queued for a device about to be woken */
	unsigned long quirks;
	bool hardflow;
};

#define CDC_DATA_INTERFACE_TYPE	0x0a

/* constants describing various quirks and errors */
#define NO_UNION_NORMAL			BIT(0)
#define SINGLE_RX_URB			BIT(1)
#define NO_CAP_LINE			BIT(2)
#define NO_DATA_INTERFACE		BIT(4)
#define IGNORE_DEVICE			BIT(5)
#define QUIRK_CONTROL_LINE_STATE	BIT(6)
#define CLEAR_HALT_CONDITIONS		BIT(7)


