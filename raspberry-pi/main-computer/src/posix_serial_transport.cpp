#include "posix_serial_transport.hpp"

#include <cerrno>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace {

speed_t baud_to_termios(std::uint32_t baud) {
    switch (baud) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
#ifdef B230400
        case 230400: return B230400;
#endif
        default: return 0;
    }
}

} // namespace

PosixSerialTransport::~PosixSerialTransport() {
    close_device();
}

bool PosixSerialTransport::open_device(const std::string& path, std::uint32_t baud) {
    close_device();

    const speed_t speed = baud_to_termios(baud);
    if (speed == 0) return false;

    fd_ = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) return false;

    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
        close_device();
        return false;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        close_device();
        return false;
    }

    tcflush(fd_, TCIOFLUSH);
    return true;
}

void PosixSerialTransport::close_device() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool PosixSerialTransport::write(const std::uint8_t* data, std::size_t length) {
    if (fd_ < 0 || data == nullptr) return false;

    std::size_t written = 0;
    while (written < length) {
        const ssize_t result = ::write(fd_, data + written, length - written);
        if (result > 0) {
            written += static_cast<std::size_t>(result);
            continue;
        }
        if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            continue;
        }
        return false;
    }
    return true;
}

std::size_t PosixSerialTransport::read(std::uint8_t* data, std::size_t capacity) {
    if (fd_ < 0 || data == nullptr || capacity == 0) return 0;

    const ssize_t result = ::read(fd_, data, capacity);
    if (result <= 0) return 0;
    return static_cast<std::size_t>(result);
}
