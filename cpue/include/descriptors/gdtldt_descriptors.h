#pragma once

#include "common.h"
#include "descriptors/descriptors.h"
#include "descriptors/concrete/segment_descriptors.h"
#include "descriptors/concrete/gate_descriptors.h"

namespace CPUE {


// See chapter 3.4.5 (page 3174)
/**
 * This is one of the following descriptors:
 * - task-gate descriptor (8 bytes, zero extended to 16 bytes)
 * - Call-gate descriptor (16 bytes)
 * - System-Segment Descriptors (TSS/LDT descriptor) (16 bytes)
 * - Application Segment descriptor (Code/Data) (still 8 bytes)
 */
struct GDTLDTDescriptor {
    // size: 128 bits
    u64 : 40;
    Descriptor::AccessByte access;
    u16 : 16;
    u64 : 64;


    bool is_gate_descriptor() const { return one_of(access.descriptor_type(), {DescriptorType::TASK_GATE, DescriptorType::CALL_GATE}); }
    bool is_system_segment_descriptor() const {
        return one_of(access.descriptor_type(), {DescriptorType::TSS_AVAILABLE_SEGMENT, DescriptorType::TSS_BUSY_SEGMENT, DescriptorType::LDT_SEGMENT});
    }
    bool is_application_segment_descriptor() const { return one_of(access.descriptor_type(), {DescriptorType::CODE_SEGMENT, DescriptorType::DATA_SEGMENT}); }

    template<typename R, typename F>
    R with_concrete_type_do(F&& f) const {
        auto type = access.descriptor_type();
        if (type == DescriptorType::CALL_GATE)
            return f((CallGateDescriptor*)this);
        if (type == DescriptorType::TASK_GATE)
            return f((TaskGateDescriptor*)this);
        if (is_system_segment_descriptor())
            return f((SystemSegmentDescriptor*)this);
        if (is_application_segment_descriptor())
            return f((ApplicationSegmentDescriptor*)this);
        fail();
    }

    u64 base() const {
        return with_concrete_type_do<u64>(overloaded{
            [&](DescriptorWithBaseAddress auto* d) -> u64 {
                return d->base();
            },
            [&](void*) -> u64 {
                fail();
            },
        });
    }
    void set_base(u64 base) {
        with_concrete_type_do<void>(overloaded{
            [&](DescriptorWithBaseAddress auto* d) {
                d->set_base(base);
            },
            [&](void*) {},
        });
    }
    u32 limit() const {
        return with_concrete_type_do<u32>(overloaded{
            [&](DescriptorWithLimit auto* d) -> u32 {
                return d->limit();
            },
            [&](void*) -> u64 {
                fail();
            },
        });
    }
    void set_limit(u32 limit) {
        with_concrete_type_do<void>(overloaded{
            [&](DescriptorWithLimit auto* d) {
                d->set_limit(limit);
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
    CallGateDescriptor* to_call_gate_descriptor() const {
        return with_concrete_type_do<CallGateDescriptor*>(overloaded{
            [&](CallGateDescriptor* d) -> CallGateDescriptor* {
                return d;
            },
            [&](void*) -> CallGateDescriptor* {
                fail("Not a call-gate");
            },
        });
    }
    SystemSegmentDescriptor* to_system_segment_descriptor() const {
        return with_concrete_type_do<SystemSegmentDescriptor*>(overloaded{
            [&](SystemSegmentDescriptor* d) -> SystemSegmentDescriptor* {
                return d;
            },
            [&](void*) -> SystemSegmentDescriptor* {
                fail("Not a system-segment descriptor");
            },
        });
    }
    ApplicationSegmentDescriptor* to_application_segment_descriptor() const {
        return with_concrete_type_do<ApplicationSegmentDescriptor*>(overloaded{
            [&](ApplicationSegmentDescriptor* d) -> ApplicationSegmentDescriptor* {
                return d;
            },
            [&](void*) -> ApplicationSegmentDescriptor* {
                fail("Not an application-segment descriptor");
            },
        });
    }
};
}