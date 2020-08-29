#include "qmsg.h"

namespace bn {

////////////////
// QMsg
////////////////

static int DEFAULT_QMSG_DATA[] = { 0, 0 };

QMsg::QMsg()
{
    m_data = DEFAULT_QMSG_DATA;
    m_size = sizeof(int) * 2;
}

QMsg::QMsg(const void* data, int dataSize)
{
    m_data = data;
    m_size = dataSize;
}

int QMsg::getCtrlTag() const
{
    return intPtr()[0];
}

int QMsg::getMsgTag() const
{
    return intPtr()[1];
}

int* QMsg::intPtr() const
{
    return (int*)m_data;
}

////////////////
// QMsgMut
////////////////

QMsgMut::QMsgMut()
    : QMsgMut(0, 0, 0)
{}

QMsgMut::QMsgMut(int ctrlTag, int dataSize)
    : QMsgMut(ctrlTag, 0, dataSize)
{}

QMsgMut::QMsgMut(int ctrlTag, int msgTag, int dataSize)
{
    resize(dataSize);
    int* pData = intPtr();
    pData[0] = ctrlTag;
    pData[1] = msgTag;
}

QMsgMut::QMsgMut(const QMsg& other)
{
    m_data = m_buf.Set((uint8_t*)other.data(), other.dataSize());
    m_size = m_buf.GetSize();
}

void QMsgMut::copyMsg(const QMsg& other)
{
    m_data = m_buf.Set((uint8_t*)other.data(), other.dataSize());
    m_size = m_buf.GetSize();
}

void QMsgMut::setCtrlTag(int v)
{
    intPtr()[0] = v;
}


void QMsgMut::setMsgTag(int v)
{
    intPtr()[1] = v;
}

void QMsgMut::setBuffer(void* data, int dataSize)
{
    resize(dataSize);
    memcpy(buffer<char>(), data, dataSize);
}

void QMsgMut::resize(int newSize)
{
    m_data = m_buf.Resize(sizeof(int) * 2 + newSize);
    m_size = m_buf.GetSize();
}

////////////////
// QMsgSender
////////////////

QMsgSender::QMsgSender(int queueSize)
    : m_queue(queueSize)
    , m_free(queueSize)
{
    // Init buffers
    for (int i = 0; i < queueSize; i++) {
        ByteBuf* bb = new ByteBuf();
        bb->Resize(16);
        m_free.Push(bb);
    }
}

QMsgSender::~QMsgSender()
{
    // First clear the queue
    while (m_queue.ElementsAvailable()) {
        WDL_HeapBuf* bb;
        m_queue.Pop(bb);
        delete bb;
    }
    // Now delete all slices
    while (m_free.ElementsAvailable()) {
        WDL_HeapBuf* bb;
        m_free.Pop(bb);
        delete bb;
    }
}

bool QMsgSender::PushData(const QMsg& msg)
{
    ByteBuf* bb = nullptr;
    // Try to get a buffer that already has enough space.
    // If we can't or we go through too many buffers, just use one.
    // This helps reduce the number of "large" buffers and thus memory usage.
    for (int i = 0; i < 2; i++) {
        // Return current buffer to the free queue.
        if (bb) {
            m_free.Push(bb);
        }
        // If there's nothing in the queue, done.
        if (!m_free.Pop(bb)) { return false; }
        // If we have enough space, exit early
        if (msg.dataSize() <= bb->GetCapacity()) {
            break;
        }
    }
    // Now that we've picked our buffer, resize it to fit (don't allow down-sizing).
    // Then copy the data into the buffer, and put the buffer into the queue.
    bb->Resize(msg.dataSize(), false);
    memcpy(bb->Get(), msg.data(), msg.dataSize());
    m_queue.Push(bb);
    return true;
}

void QMsgSender::TransmitData(iplug::IEditorDelegate& editor)
{
    // Send all messages to the UI
    while (m_queue.ElementsAvailable()) {
        WDL_HeapBuf* bb;
        m_queue.Pop(bb);
        QMsg msg(bb->Get(), bb->GetSize());
        editor.SendControlMsgFromDelegate(msg.getCtrlTag(), msg.getMsgTag(), msg.dataSize(), msg.data());
        m_free.Push(bb);
    }
}

}
