//
//  BitOptions.h
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 2017-11-11.
//  Revised by FireWolf on 2019-09-27.
//  Adopted by FireWolf on 2021-02-19. (Apple Darwin Kernel)
//
//  OSBitOptions, the original name, is part of OSFoundation.
//  The header was initially implemented for CPSC 415 Xeros OS.
//  BitOptions has now been optimized for C++.
//  Copyright © 2017-2020 FireWolf. All rights reserved.
//

#ifndef BitOptions_h
#define BitOptions_h

#include <libkern/OSTypes.h>
#include "Debug.hpp"

//
// OSBitOptions - The original header
//
// A lightweight "interface" to represent bit mask types
// Each bit is a member of the options
//
// `OSBitOptions` treats an integer as a bit mask and provides
// useful functions to manipulate individual members.
//
// For example, an `UInt8` type can be used to represent 8 different options.
// The index of the highest bit is 7, whereas the lowest bit is 0.
//
// BINARY: 1 0 0 0 1 1 1 1
//  INDEX: 7 6 5 4 3 2 1 0
//
// A useful macro `OSBitOptionCreate(index)` takes the bit index as its argument
// and create the corresponding option value.
//

///
/// Represents a collection of bit options
///
/// @note Contains up to sizeof(T) options
/// @note It is recommended to use `uint8_t`, `uint16_t`, `uint32_t` and `uint64_t` as the storage type.
///       If variable size is desired, consider to use `size_t` or `unsigned long` instead.
///       By default, it uses the `size_t` type to best work with both 32-bit and 64-bit platforms.
///
template <typename T = size_t>
struct BitOptions
{
private:
    /// The internal storage
    T options;

public:
    /// An unsigned integer type that represents a single bit option
    using BitOption = T;

    ///
    /// Create a single bit option with the given bit index
    ///
    /// @param index The index of the bit
    /// @return The BitOption value.
    ///
    static inline BitOption createOptionWithIndex(size_t index)
    {
        //passert(index < sizeof(T) * 8, "The given bit index is invalid.");

        return static_cast<T>(1) << index;
    }

    /// Create a collection of bit options from the given raw value
    /// By default, an empty collection of options is created.
    BitOptions(T options = 0)
    {
        this->options = options;
    }

    ///
    /// Insert the given option
    ///
    /// @param option An option to be inserted
    ///
    inline void mutativeInsert(BitOption option)
    {
        this->options |= option;
    }

    ///
    /// Remove the given option
    ///
    /// @param option An option to be removed
    ///
    inline void mutativeRemove(BitOption option)
    {
        this->options &= (~option);
    }

    ///
    /// Perform the bitwise AND operation with the given option
    ///
    /// @param option An option to be "and"ed
    ///
    inline void mutativeBitwiseAnd(BitOption option)
    {
        this->options &= option;
    }

    ///
    /// Perform the bitwise OR operation with the given option
    ///
    /// @param option An option to be "or"ed
    ///
    inline void mutativeBitwiseOr(BitOption option)
    {
        this->options |= option;
    }

    ///
    /// Insert the given option and return the new options
    ///
    /// @param option An option to be inserted
    /// @return The new options after the insertion
    ///
    [[nodiscard]]
    inline BitOptions insert(BitOption option) const
    {
        return BitOptions(this->options | option);
    }

    ///
    /// Remove the given option and return the new options
    ///
    /// @param option An option to be removed
    /// @return The new options after the removal
    ///
    [[nodiscard]]
    inline BitOptions remove(BitOption option) const
    {
        return BitOptions(this->options & (~option));
    }

    ///
    /// Perform the bitwise AND operation with the given option and return the new options
    ///
    /// @param option An option to be "and"ed
    ///
    [[nodiscard]]
    inline BitOptions bitwiseAnd(BitOption option) const
    {
        return BitOptions(this->options & option);
    }

    ///
    /// Perform the bitwise OR operation with the given option and return the new options
    ///
    /// @param option An option to be "or"ed
    ///
    [[nodiscard]]
    inline BitOptions bitwiseOr(BitOption option) const
    {
        return BitOptions(this->options | option);
    }

    ///
    /// Check whether the given option in in this collection
    ///
    /// @param option An option to be checked
    /// @return `true` if `this` contains the given `option`, `false` otherwise.
    ///
    [[nodiscard]]
    inline bool contains(BitOption option) const
    {
        return (this->options & option) == option;
    }
    
    ///
    /// Check whether this collection of bit options is empty
    ///
    /// @return `true` if it is empty, `false` otherwise.
    ///
    [[nodiscard]]
    inline bool isEmpty() const
    {
        return this->options == 0;
    }
    
    ///
    /// Export this collection of options as a single option
    ///
    /// @return The flatten version of this collection of options.
    ///
    [[nodiscard]]
    inline BitOption flatten() const
    {
        return this->options;
    }

    ///
    /// Clear all bits
    ///
    inline void clearAll()
    {
        this->options = 0;
    }

    ///
    /// Set all bits
    ///
    inline void setAll()
    {
        this->options = -1;
    }

    ///
    /// Check whether the bit at the given index is set
    ///
    /// @param index Index of the bit
    /// @return `true` if the target bit is set, `false` otherwise.
    ///
    [[nodiscard]]
    inline bool containsBit(size_t index) const
    {
        return this->contains(BitOptions<T>::createOptionWithIndex(index));
    }

    ///
    /// Get the bit at the given index
    ///
    /// @param index Index of the bit
    /// @return The bit at the given index.
    ///
    [[nodiscard]]
    inline uint8_t getBit(size_t index) const
    {
        return this->containsBit(index) ? 1 : 0;
    }

    ///
    /// Set the bit at the given index
    ///
    /// @param index Index of the bit
    ///
    inline void setBit(size_t index)
    {
        this->mutativeInsert(BitOptions<T>::createOptionWithIndex(index));
    }

    ///
    /// Clear the bit at the given index
    ///
    /// @param index Index of the bit
    ///
    inline void clearBit(size_t index)
    {
        this->mutativeRemove(BitOptions<T>::createOptionWithIndex(index));
    }

    /// Perform the bitwise AND and the assignment operation
    BitOptions& operator&=(const BitOptions& rhs)
    {
        this->options &= rhs.options;

        return *this;
    }

    /// Perform the bitwise OR and the assignment operation
    BitOptions& operator|=(const BitOptions& rhs)
    {
        this->options |= rhs.options;

        return *this;
    }

    /// Perform the bitwise AND operation
    BitOptions operator&(const BitOptions& rhs)
    {
        return BitOptions(this->options & rhs.options);
    }

    /// Perform the bitwise AND operation
    BitOptions operator|(const BitOptions& rhs)
    {
        return BitOptions(this->options | rhs.options);
    }
};

#endif /* BitOptions_h */
