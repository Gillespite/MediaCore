#ifndef STOP_STREAM_H_
#define STOP_STREAM_H_

#include <zlog.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace MediaCoreMessageFormat
{
	struct __attribute__((packed)) StopStreamMessage
	{
		uint32_t	type;
		uint32_t	length;
		uint32_t	tid;
		uint32_t	ssrc;
	};

	void InitStopStreamMessage(StopStreamMessage* message)
	{
		message->type	= 0x36;
		message->length	= 0x04;
		message->tid	= 0x00;
		message->ssrc	= 0x00;
	}

	bool SendStopStreamMessage(int Socket, const StopStreamMessage* message)
	{
		{
			if (Socket < 0) 
			{
				dzlog_error("socketfd: %d", Socket);
				return false;
			}

			if (message == nullptr) 
			{
				dzlog_error("message == nullptr");
				return false;
			}
		}

		int length	= sizeof(StopStreamMessage);
		int once	= 0;

		for (int i = 0; i != length; i += once)
		{
			once = send(Socket, (const uint8_t*)message + i, length - i, MSG_CONFIRM);

			if (once == -1)
			{
				dzlog_error("errno: %d", errno);
				return false;
			}
		}

		dzlog_info("success for socket: %d", Socket);
		return true;
	}

	int SetStopStreamData(StopStreamMessage* message, uint32_t ssrc)
	{
		{
			if (message == nullptr)
			{
				dzlog_error("message == nullptr");
				return -1;
			}

			if (ssrc <= 0)
			{
				dzlog_error("send port <= 0");
				return -2;
			}
		}

		message->ssrc = ssrc;

		return 0;
	}
}

#endif