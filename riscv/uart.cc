
#include <exception>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <uart.h>

bool uart_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
    bool ret = true;

    switch (addr >> 3)
    {
    case 0: // RBR, DLL
	if (lcr & 0x80)
	    bytes[0] = dll;
	else {
	    int c = rx();
	    bytes[0] = c == -1 ? 0 : c;
	}
	break;
    case 1: // IER, DLM
	if (lcr & 0x80)
	    bytes[0] = dlm;
	else
	    bytes[0] = ier;
	break;
    case 2: // IIR, EFR
	if (lcr == 0xbf)
	    bytes[0] = efr;
	else {
	    bytes[0] = iir;
	    clear_interrupt();
	}
	break;
    case 3: // LCR
	bytes[0] = lcr;
	break;
    case 4: // MCR
	bytes[0] = mcr;
	break;
    case 5: // LSR
	lsr = (rx_ready() ? 1 : 0) | 0x60;
	bytes[0] = lsr;
	break;
    case 6: // MSR
	bytes[0] = msr;
	break;
    case 7: // SCR
	bytes[0] = scr;
	break;
    default:
	ret = false;
	break;
    }

    return ret;
}

bool uart_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
    //printf("UART: STORE: addr=%lx\n", addr);
    bool ret = true;

    switch (addr >> 3)
    {
    case 0: // THR, DLL
	if (lcr & 0x80)
	    dll = bytes[0];
	else {
	    tx(bytes[0]);
	    if (ier & 0x2)
		raise_interrupt();
	}
	break;
    case 1: // IER, DLM
	if (lcr & 0x80)
	    dlm = bytes[0];
	else
	    ier = bytes[0];
	break;
    case 2: // FCR, EFR
	if (lcr == 0xbf)
	    efr = bytes[0];
	else
	    fcr = bytes[0];
	break;
    case 3: // LCR
	lcr = bytes[0];
	break;
    case 4: // MCR
	mcr = bytes[0];
	break;
    case 5: // LSR
	//lsr = bytes[0];  // read only
	break;
    case 6: // MSR
	//msr = bytes[0];  // read only
	break;
    case 7: // SCR
	scr = bytes[0];
	break;
    default:
	ret = false;
	break;
    }

    return ret;
}

uart_t::uart_t(processor_t* proc) : proc(proc)
{
    // initialize registers
    ier = 0;
    iir = 1;
    lcr = 0;
    mcr = 0;
    lsr = 0;
    msr = 0;
    scr = 0;
    dll = 0;
    dlm = 0;
    efr = 0;

    // open pty
    fd = posix_openpt(O_RDWR | O_NOCTTY);

    if (fd == -1) {
	throw std::runtime_error(strerror(errno));
    }

    if (grantpt(fd) == -1) {
	throw std::runtime_error(strerror(errno));
    }

    if (unlockpt(fd) == -1) {
	throw std::runtime_error(strerror(errno));
    }

    struct termios t;

    if (tcgetattr(fd, &t) == -1) {
	throw std::runtime_error(strerror(errno));
    }

    cfmakeraw(&t);

    if (tcsetattr(fd, TCSANOW, &t) == -1) {
	throw std::runtime_error(strerror(errno));
    }
}

uart_t::~uart_t()
{
    close(fd);

    if (!link.empty())
    {
	unlink(link.c_str());
    }
}

void uart_t::create_link(const std::string& link)
{
    printf("UART: CREATE_LINK: %s\n", link.c_str());
    // only allow one link, and disallow empty link
    if (!this->link.empty() || link.empty())
	return;

    this->link = link;

    char name[PATH_MAX];
    if (ptsname_r(fd, name, sizeof(name)) != 0) {
	throw std::runtime_error(strerror(errno));
    }

    unlink(link.c_str());

    if (symlink(name, link.c_str()) == -1) {
	throw std::runtime_error(strerror(errno));
    }
}

void uart_t::tick(void)
{
    if ((ier & 0x1) && rx_ready())
	raise_interrupt();
}
