#pragma once

#include "common.h"
#include "descriptors/descriptors.h"
#include "descriptors/concrete/gate_descriptors.h"

namespace CPUE {
// See chapter 3.4.5 (page 3174)
/**
 * This is one of the following descriptors:
 * - Task-gate descriptor (8 bytes, zero extended to 16 bytes)
 * - Interrupt-gate descriptor (16-bytes)
 * - Trap-gate descriptor (16-bytes)
 */
struct IDTDescriptor {
    // size: 128 bits
    u64 : 40;
    Descriptor::AccessByte access;
    u16 : 16;
    u64 : 64;


    template<typename R, typename F>
    R with_concrete_type_do(F&& f) const {
        auto type = access.descriptor_type();
        if (type == DescriptorType::TASK_GATE)
            return f((TaskGateDescriptor*)this);
        if (type == DescriptorType::INTERRUPT_GATE)
            return f((InterruptGateDescriptor*)this);
        if (type == DescriptorType::TRAP_GATE)
            return f((TrapGateDescriptor*)this);
        fail();
    }

    SegmentSelector segment_selector() const {
        return with_concrete_type_do<u64>(overloaded{
            [&](DescriptorWithSegmentSelector auto* d) -> u64 {
                return d->segment_selector;
            },
            [&](void*) -> u64 {
                fail();
            },
        });
    }
    void set_segment_selector(SegmentSelector selector) {
        with_concrete_type_do<void>(overloaded{
            [&](DescriptorWithSegmentSelector auto* d) {
                d->segment_selector = selector;
            },
            [&](void*) {},
        });
    }
    u32 offset() const {
        return with_concrete_type_do<u32>(overloaded{
            [&](DescriptorWithNoOffset auto*) -> u32 {
                fail();
            },
            [&](DescriptorWithOffset auto* d) -> u32 {
                return d->offset();
            },
        });
    }
    void set_offset(u32 offset) {
        with_concrete_type_do<void>(overloaded{
            [&](DescriptorWithNoOffset auto* d) {},
            [&](DescriptorWithOffset auto* d) {
                d->set_offset(offset);
            },
        });
    }


    TaskGateDescriptor* to_task_gate_descriptor() const {
        return with_concrete_type_do<TaskGateDescriptor*>(overloaded{
            [](TaskGateDescriptor* d) -> TaskGateDescriptor* {
                return d;
            },
            [](void*) -> TaskGateDescriptor* {
                fail("Not a task-gate");
            },
        });
    }
    InterruptGateDescriptor* to_interrupt_gate_descriptor() const {
        return with_concrete_type_do<InterruptGateDescriptor*>(overloaded{
            [&](InterruptGateDescriptor* d) -> InterruptGateDescriptor* {
                return d;
            },
            [&](void*) -> InterruptGateDescriptor* {
                fail("Not an interrupt-gate");
            },
        });
    }
    TrapGateDescriptor* to_trap_gate_descriptor() const {
        return with_concrete_type_do<TrapGateDescriptor*>(overloaded{
            [&](TrapGateDescriptor* d) -> TrapGateDescriptor* {
                return d;
            },
            [&](void*) -> TrapGateDescriptor* {
                fail("Not a trap-gate descriptor");
            },
        });
    }
};

}
