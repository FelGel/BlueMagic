// TcpSocketTest.h
//////////////////////////////////////////////

#pragma once



//#include "GeneralTcpInterface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#define THIS_FILE __FILE__
#endif


///////////////////////////////////////////////////////////////////
// Test
///////////////////////////////////////////////////////////////////

#ifdef _TEST
#include "Common\LogEvent.h"
inline int Random(int Max)
{
    return (rand() % 11335577) % Max;
}

enum { MAX_DATA_SIZE = 5000 };

struct TestMessage
{
    int MessageOpcode;
    int DataSize;
    enum { 
        OPCODE = 0x12345678,
    };
    BYTE Data[MAX_DATA_SIZE];

    static inline int HeaderSize()
    {
        static TestMessage* Message;
        return sizeof TestMessage - sizeof Message->Data;
    }

    // Prepare message and return actual size
    int Fill(int Start, int Size)
    {
        if (Size > MAX_DATA_SIZE)
            Size = MAX_DATA_SIZE;

        MessageOpcode = OPCODE;
        DataSize = Size;

        for (int i = 0; i < DataSize; ++i)
            Data[i] = (BYTE)(Start + i);

        // return Total size
        return HeaderSize() + DataSize;
    }

    bool Check(int& Start, int TotalSize) const
    {
        if (TotalSize < HeaderSize())
        {
            LogEvent(LE_ERROR, 
                "TestMessage::Check: Total size too short (%d < %d)",
                TotalSize, HeaderSize()); 
            return false;
        }
        if (MessageOpcode != OPCODE)
        {
            LogEvent(LE_ERROR, 
                "TestMessage::Check: Illegal opcode (%d != %d)", 
                MessageOpcode, OPCODE); 
            return false;
        }

        if (TotalSize != DataSize + HeaderSize())
        {
            LogEvent(LE_ERROR,
                "TestMessage::Check: Illegal size (%d != %d + %d)",
                TotalSize, DataSize, HeaderSize());
            return false;
        }

        for (int i = 0; i < DataSize; ++i)
        {
            if (Data[i] != (BYTE)(Start + i))
            {
                LogEvent(LE_ERROR,
                    "TestMessage::Check: Illegal data (%d != %d)",
                    Data[i], (BYTE)(Start + i));

                if (i == 0)
                    Start = Data[i]; //try to sync
                else
                    return false;
            }
        }
        return true;
    }
};

#include "TcpSocketExtensions.h"
class CTestMessageParser : public CBasicTcpParser
{
public:
    CTestMessageParser(): CBasicTcpParser(sizeof TestMessage)
    {
    }

    // Returns true on a whole message - actual returns the size of the message
    virtual bool ParseMessage(const BYTE* Data, int Length, int& Actual)
    {
        if (Length < TestMessage::HeaderSize())
            return false;

        const TestMessage* Message = (const TestMessage*)Data;
        int RequestedLength = TestMessage::HeaderSize() + Message->DataSize;

        Actual = RequestedLength;
        return Length >= RequestedLength;
    }

    virtual ITcpIncomingFilter* CloneIncomingFilter()
    {
        return new CTestMessageParser();
    }
};


//////////////////////////////////////////////////////////////////////////
// AMS TEST 
//////////////////////////////////////////////////////////////////////////
//#pragma pack (push, TestAms, 1)

//enum TEST_AMS_MESSAGE_OPCODE
//{
//	AMS_TEST_AMS_MESSAGE	= 0x1000,
//	AMS_TEST_DMM_MESSAGE
//};
//
//enum{MAX_RAW_DATA = 10000};
//struct TestAmsMessage : public BaseAmsMessage
//{
//    TestAmsMessage(WORD _RawDataSize, /*const char *_RawData,*/WORD RequestId=0) : 
//		BaseAmsMessage(AMS_TEST_AMS_MESSAGE, sizeof *this, RequestId),
//		RawDataSize(_RawDataSize)
//		{
//			for(WORD i=0 ; i<RawDataSize ; ++i)
//				RawData[i] = i; //_RawData[i];
//			UpdateMessageLength();
//		}
//
//	WORD	RawDataSize;		//	Should be non-zero if and only if Quality 
//									// Type is CRC Valid                                                                                            
//	char	RawData[MAX_RAW_DATA];              
//
//	void UpdateMessageLength(){Header.TotalLength = ACTUAL_SIZE(RawData, RawDataSize);}
//};
//
//
//struct TestDmmMessage : public BaseAmsMessage
//{
//    TestDmmMessage(WORD _Channel, WORD _Trnasponder, 
//		WORD _Number, WORD RequestId=0) : 
//		BaseAmsMessage(AMS_TEST_DMM_MESSAGE, sizeof *this, RequestId),
//		Channel(_Channel),
//		Trnasponder(_Trnasponder),
//		Number(_Number)
//		{
//		}
//
//	WORD	Channel;
//	WORD	Trnasponder;
//	
//	WORD	Number;
//};

//#pragma pack (pop, TestAms)

#endif


#ifdef _DEBUG
#define THIS_FILE __FILE__
#undef new
#endif
