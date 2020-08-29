#pragma once
#include <cstdint>
#include <vector>
#include "heapbuf.h"
#include "IPlugEditorDelegate.h"
#include "IPlugQueue.h"

namespace bn {

class QMsg
{
public:
    QMsg();
    QMsg(const void* data, int size);

    int getCtrlTag() const;
    int getMsgTag() const;

    template<typename T>
    inline T* buffer()
    {
        return (T*)((uint8_t*)m_data + sizeof(int) * 2);
    }

    inline int size() const { return m_size - sizeof(int) * 2; }
    inline const void* data() const { return m_data; }
    inline int dataSize() const { return m_size; }

protected:
    int* intPtr() const;

    const void* m_data;
    int m_size;
};

/** A mutable version of QMsg which owns its data buffer. */
class QMsgMut : public QMsg
{
public:
    QMsgMut();
    QMsgMut(int ctrlTag, int dataSize);
    QMsgMut(int ctrlTag, int msgTag, int dataSize);
    QMsgMut(const QMsg& other);

    /** Copy the other QMsg's data into this message */
    void copyMsg(const QMsg& other);

    /** Set the control tag for this QMsg */
    void setCtrlTag(int v);

    /** Set the message tag for this QMsg */
    void setMsgTag(int v);

    /** Set the buffer data for this QMsg (does NOT change the control or message tags) */
    void setBuffer(void* data, int dataSize);

    /** Resize the data portion of the QMsg */
    void resize(int newSize);

    WDL_TypedBuf<uint8_t> toBuffer() const
    {
        WDL_TypedBuf<uint8_t> bb;
        bb.Set((uint8_t*)data(), dataSize());
        return bb;
    }

protected:
    WDL_TypedBuf<uint8_t> m_buf;
};

using iplug::IPlugQueue;

/** QMsgSender is a utility class which can be used to send data from the realtime audio thread to the GUI */
class QMsgSender
{
public:
    QMsgSender(int queueSize);
    ~QMsgSender();

    /** Pushes a data element onto the queue. This can be called on the realtime audio thread. */
    bool PushData(const QMsg& msg);

    /** Pops elements off the queue and sends messages to controls.
        *  This must be called on the main thread - typically in MyPlugin::OnIdle() */
    void TransmitData(iplug::IEditorDelegate& editor);

private:
    using ByteBuf = WDL_HeapBuf;
    IPlugQueue<ByteBuf*> m_free;
    IPlugQueue<ByteBuf*> m_queue;
};

} // end namespace bn
