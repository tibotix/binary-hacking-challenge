#include "register_proxy.h"
#include "cpu.h"

namespace CPUE {


InterruptRaisedOr<void> RegisterProxy::before_read() const {
    if (!(m_flags & REG_ACCESS_READ))
        return m_cpu->raise_integral_interrupt(Exceptions::UD());
    return m_callbacks.invoke_before_read();
}
InterruptRaisedOr<void> RegisterProxy::before_write() const {
    if (!(m_flags & REG_ACCESS_WRITE))
        return m_cpu->raise_integral_interrupt(Exceptions::UD());
    return m_callbacks.invoke_before_write();
}


InterruptRaisedOr<SizedValue> GeneralPurposeRegisterProxy::do_read() const {
    return m_value_modifier.read(m_value_ptr);
}
InterruptRaisedOr<void> GeneralPurposeRegisterProxy::do_write(SizedValue const& value) {
    m_value_modifier.write(m_value_ptr, value);
    return {};
}

InterruptRaisedOr<SizedValue> ControlRegisterProxy::do_read() const {
    return RegisterValueModifier::QWORD.read(m_value_ptr);
}
InterruptRaisedOr<void> ControlRegisterProxy::do_write(SizedValue const& value) {
    RegisterValueModifier::QWORD.write(m_value_ptr, value);
    return {};
}


InterruptRaisedOr<SizedValue> ApplicationSegmentRegisterProxy::do_read() const {
    return m_seg_ptr->visible.segment_selector.value;
}
InterruptRaisedOr<void> ApplicationSegmentRegisterProxy::do_write(SizedValue const& value) {
    return m_cpu->load_segment_register(m_seg_alias, SegmentSelector(value.value()));
}




}