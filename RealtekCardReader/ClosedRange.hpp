//
//  ClosedRange.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 9/25/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef ClosedRange_hpp
#define ClosedRange_hpp

#include <libkern/OSTypes.h>
#include "Debug.hpp"

/// A structure that describes a closed interval from a lower bound to an upper bound
template <typename Bound = UInt32>
struct ClosedRange
{
    /// The lower bound
    Bound lowerBound;

    /// The upper bound
    Bound upperBound;
    
    /// Default constructor
    ClosedRange() {}

    /// Create a closed range with the given bounds
    ClosedRange(Bound lowerBound, Bound upperBound)
    {
        passert(lowerBound <= upperBound, "The given bounds are invalid.");

        this->lowerBound = lowerBound;

        this->upperBound = upperBound;
    }

    ///
    /// Create a closed range with the given length
    ///
    /// @param start The start position as the lower bound
    /// @param length The number of elements in the range
    /// @return A closed range that starts at `start` and covers `length` elements.
    ///
    static inline ClosedRange createWithLength(Bound start, Bound length)
    {
        return ClosedRange(start, start + length - 1);
    }

    /// Get the number of elements in this range
    [[nodiscard]]
    inline Bound getLength() const
    {
        return this->upperBound - this->lowerBound + 1;
    }

    ///
    /// Check whether this range is valid
    ///
    /// @return `true` if valid, `false` otherwise.
    ///
    [[nodiscard]]
    inline bool isValid() const
    {
        return this->lowerBound <= this->upperBound;
    }
    
    ///
    /// Check whether the range contains the given value
    ///
    /// @param value The value to be checked
    /// @return `true` if the value is within the range, `false` otherwise.
    ///
    [[nodiscard]]
    inline bool contains(Bound value) const
    {
        return this->lowerBound <= value && value <= this->upperBound;
    }
};

#endif /* ClosedRange_hpp */
