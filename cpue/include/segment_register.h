#pragma once

#include "segmentation.h"
#include "descriptors/concrete/segment_descriptors.h"

namespace CPUE {


/**
 * This register is only allowed to store LDD/TSS Segment Descriptors (aka SystemSegmentDescriptors).
 */
struct SystemSegmentRegister {
    // visible part:
    struct {
        SegmentSelector segment_selector{0};
    } visible;
    /**
     * hidden part (shadow register):
     * When a segment selector is loaded into the visible part of a segment
     * register, the processor also loads the hidden part of the segment register with the base address, segment limit, and
     * access control information from the segment descriptor pointed to by the segment selector. The information cached
     * in the segment register (visible and hidden) allows the processor to translate addresses without taking extra bus
     * cycles to read the base address and limit from the segment descriptor
     *
     * NOTE: We just store the full segment descriptor for simplicity.
     *
     */
    struct {
        SystemSegmentDescriptor cached_descriptor;
    } hidden;
};


/**
 * ApplicationSegment Registers:
 * In 64-bit mode: CS, DS, ES, SS are treated as if each segment base is 0, regardless of the value of the associated
 * segment descriptor base. This creates a flat address space for code, data, and stack. FS and GS are exceptions.
 * Both segment registers may be used as additional base registers in linear address calculations
 *
 * This register is only allowed to store Data/Code Segment Descriptors (aka ApplicationSegmentDescriptors).
 */
struct ApplicationSegmentRegister {
    struct {
        SegmentSelector segment_selector{0};
    } visible;
    struct {
        ApplicationSegmentDescriptor cached_descriptor;
    } hidden;
};



enum SegmentRegisterType {
    CODE,
    DATA,
    STACK,
    SYSTEM,
};

class SegmentRegisterAlias {
public:
    // DO NOT CHANGE THE ORDER OF THESE!!!
    enum Alias : int { CS = 0, DS, SS, ES, FS, GS, LDTR, TR };

    SegmentRegisterAlias() = default;
    constexpr SegmentRegisterAlias(Alias alias) : m_alias(alias) {}

    // Allow switch and comparisons.
    constexpr operator Alias() const { return m_alias; }

    // Prevent usage in if statement
    explicit operator bool() const = delete;

    constexpr SegmentRegisterType type() const {
        switch (m_alias) {
            case Alias::CS: return SegmentRegisterType::CODE;
            case Alias::DS: return SegmentRegisterType::DATA;
            case Alias::SS: return SegmentRegisterType::STACK;
            case Alias::ES:
            case Alias::FS:
            case Alias::GS: return SegmentRegisterType::DATA;
            case Alias::LDTR:
            case Alias::TR: return SegmentRegisterType::SYSTEM;
            default: fail();
        }
    }

private:
    Alias m_alias;
};

}