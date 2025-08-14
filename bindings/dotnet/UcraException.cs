using System;
using UCRA.Interop;

namespace UCRA
{
    /// <summary>
    /// Exception thrown when UCRA operations fail
    /// </summary>
    public class UcraException : Exception
    {
        /// <summary>
        /// Gets the UCRA error code
        /// </summary>
        public NativeMethods.UCRAResult ErrorCode { get; }

        /// <summary>
        /// Initializes a new instance of the UcraException class
        /// </summary>
        /// <param name="errorCode">The UCRA error code</param>
        public UcraException(NativeMethods.UCRAResult errorCode)
            : base(GetErrorMessage(errorCode))
        {
            ErrorCode = errorCode;
        }

        /// <summary>
        /// Initializes a new instance of the UcraException class
        /// </summary>
        /// <param name="errorCode">The UCRA error code</param>
        /// <param name="message">Custom error message</param>
        public UcraException(NativeMethods.UCRAResult errorCode, string message)
            : base($"{message} (Error code: {errorCode})")
        {
            ErrorCode = errorCode;
        }

        /// <summary>
        /// Initializes a new instance of the UcraException class
        /// </summary>
        /// <param name="errorCode">The UCRA error code</param>
        /// <param name="message">Custom error message</param>
        /// <param name="innerException">Inner exception</param>
        public UcraException(NativeMethods.UCRAResult errorCode, string message, Exception innerException)
            : base($"{message} (Error code: {errorCode})", innerException)
        {
            ErrorCode = errorCode;
        }

        /// <summary>
        /// Converts an error code to a human-readable message
        /// </summary>
        /// <param name="errorCode">The error code</param>
        /// <returns>Human-readable error message</returns>
        private static string GetErrorMessage(NativeMethods.UCRAResult errorCode)
        {
            return errorCode switch
            {
                NativeMethods.UCRAResult.Success => "Success",
                NativeMethods.UCRAResult.InvalidArgument => "Invalid argument",
                NativeMethods.UCRAResult.OutOfMemory => "Out of memory",
                NativeMethods.UCRAResult.NotSupported => "Not supported",
                NativeMethods.UCRAResult.Internal => "Internal error",
                NativeMethods.UCRAResult.FileNotFound => "File not found",
                NativeMethods.UCRAResult.InvalidJson => "Invalid JSON",
                NativeMethods.UCRAResult.InvalidManifest => "Invalid manifest",
                _ => $"Unknown error ({(uint)errorCode})"
            };
        }
    }

    /// <summary>
    /// Utility class for UCRA error handling
    /// </summary>
    internal static class ErrorHelper
    {
        /// <summary>
        /// Checks the result and throws an exception if it indicates an error
        /// </summary>
        /// <param name="result">The result to check</param>
        /// <exception cref="UcraException">Thrown if result indicates an error</exception>
        public static void CheckResult(NativeMethods.UCRAResult result)
        {
            if (result != NativeMethods.UCRAResult.Success)
            {
                throw new UcraException(result);
            }
        }

        /// <summary>
        /// Checks the result and throws an exception with a custom message if it indicates an error
        /// </summary>
        /// <param name="result">The result to check</param>
        /// <param name="message">Custom error message</param>
        /// <exception cref="UcraException">Thrown if result indicates an error</exception>
        public static void CheckResult(NativeMethods.UCRAResult result, string message)
        {
            if (result != NativeMethods.UCRAResult.Success)
            {
                throw new UcraException(result, message);
            }
        }
    }
}
