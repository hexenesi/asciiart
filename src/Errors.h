#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>
#include <string>

/** The image file is missing, unreadable or in an unsupported format. */
class ImageLoadError : public std::runtime_error {
public:
    ImageLoadError(const std::string& path, const std::string& reason)
        : std::runtime_error("cannot load image '" + path + "': " + reason), m_path(path) {}

    const std::string& path() const { return m_path; }

private:
    std::string m_path;
};

/**
 * Page settings or art size cannot produce a valid page layout (e.g. font too large for the
 * paper, glue flap wider than the space around the art). Derives from std::invalid_argument
 * because it always stems from bad input.
 */
class LayoutError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

#endif // ERRORS_H
