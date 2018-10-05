
#ifndef _UART_H
#define _UART_H

#include <assert.h>
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

#include "devices.h"
#include "processor.h"

class uart_t : public abstract_device_t
{
public:
    bool load(reg_t addr, size_t len, uint8_t* bytes);
    bool store(reg_t addr, size_t len, const uint8_t* bytes);
    uart_t(processor_t* proc);
    ~uart_t();
    void create_link(const std::string& link);
    void tick(void);
private:
    processor_t* proc;
    int fd;
    std::string link;

    uint8_t rbr;
    uint8_t ier;
    uint8_t iir;
    uint8_t fcr;
    uint8_t efr;
    uint8_t lcr;
    uint8_t mcr;
    uint8_t lsr;
    uint8_t msr;
    uint8_t scr;
    uint8_t dll;
    uint8_t dlm;

    bool rx_ready(void)
    {
	struct pollfd fds[] = { fd, POLLIN };
	int c = poll(fds, 1, 0);

	return (c == 1) && (fds[0].revents & POLLIN);
    }

    int rx(void)
    {
	if (rx_ready()) {
	    uint8_t buf[1];
	    int c = read(fd, &buf, sizeof(buf));
	    return c == 1 ? buf[0] : -1;
	}

	return -1;
    }

    void tx(int c)
    {
	uint8_t buf[] = {(uint8_t)(c & 0xff)};
	assert(write(fd, &buf, sizeof(buf)) == sizeof(buf));
    }

    void raise_interrupt(void)
    {
	iir &= ~1;
	//proc->get_state()->mip |= MIP_SEIP;
	//proc->get_state()->mip |= MIP_MEIP;
    }

    void clear_interrupt(void)
    {
	iir |= 1;
	//proc->get_state()->mip &= ~MIP_SEIP;
	//proc->get_state()->mip &= ~MIP_MEIP;
    }
};

#endif
