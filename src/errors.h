/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#ifndef ERRORS_H
#define ERRORS_H

#include <string>

/**
 * @brief Represents an error class for handling exceptions specific to the XiLens application.
 *
 * This class is used to encapsulate and manage error details, including error codes,
 * messages, and other related properties associated with errors in the XiLens application.
 * It provides an interface for retrieving error information and debugging issues
 * encountered during application runtime.
 */
class XiLensError final : public std::exception
{
  public:
    enum class Code
    {
        None,
        FileInconsistentMetadata
    };

    /**
     * Constructs a XiLensError object with a specific error code and associated error message.
     *
     * @param code The error code indicating the type of error. Default value is Code::None.
     * @param message A human-readable message providing additional details about the error. Default value is an empty
     * string.
     * @return An instance of XiLensError initialized with the provided code and message.
     */
    explicit XiLensError(const Code code = Code::None, const std::string &message = "")
        : m_code(code), m_message(message)
    {
    }

    /**
     * @brief Queries the error code.
     *
     * @return TThe error code.
     */
    Code code() const
    {
        return m_code;
    }

    /**
     * @brief Queries the error message corresponding to the error.
     *
     * @return error message.
     */
    const std::string &message() const
    {
        return m_message;
    }

    /**
     * @brief Converts error code and additional message into a human-readable string.
     *
     * @return human readable error code representation + additional error message.
     */
    std::string toString() const
    {
        return errorCodeToString(m_code) + " " + m_message;
    }

    /**
     * @brief Converts an error code into a string representation.
     *
     * @param code error code.
     * @return string representation of the error code.
     */
    static std::string errorCodeToString(const Code code)
    {
        switch (code)
        {
        case Code::FileInconsistentMetadata:
            return "File metadata is missing or has inconsistent shape.";
        default:
            return "Unknown";
        }
    }

  private:
    /**
     * @brief The error code.
     */
    Code m_code;

    /**
     * @brief The error message.
     */
    std::string m_message;
};
#endif // ERRORS_H
