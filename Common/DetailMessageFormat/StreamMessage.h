#ifndef STREAM_MESSAGE_H_
#define STREAM_MESSAGE_H_

#include <zlog.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace MediaCoreMessageFormat
{
	struct __attribute__((packed)) StreamMessage
	{
		uint32_t	type;
		uint32_t	length;
		uint32_t	tid;
		uint32_t	y_length;
		uint32_t	u_length;
		uint32_t	v_length;
		uint8_t*	y;
		uint8_t*	u;
		uint8_t*	v;
	};

	void InitStreamMessage(StreamMessage* message)
	{
		message->type		= MSGTYPE_STREAMFRAME;
		message->length		= 0x00;
		message->tid		= 0x00;
		message->y_length	= 0x00;
		message->u_length	= 0x00;
		message->v_length	= 0x00;
		message->y			= nullptr;
		message->u			= nullptr;
		message->v			= nullptr;
	}

	void PrintMessage(StreamMessage* message)
	{
		if (message == nullptr)
		{
			dzlog_error("message == nullptr");
			return;
		}

		dzlog_info("StreamMessage(type: [%d], length: [%d], tid: [%d], \ny: %s\nu: %s\nv: %s)", message->type, message->length, message->tid, (char*)message->y, (char*)message->u, (char*)message->v);
	}

	bool SendStreamMessage(int Socket, const StreamMessage* message)
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

			if (message->y == nullptr)
			{
				dzlog_error("message->y == nullptr");
				return false;
			}

			if (message->u == nullptr)
			{
				dzlog_error("message->u == nullptr");
				return false;
			}

			if (message->v == nullptr)
			{
				dzlog_error("message->v == nullptr");
				return false;
			}
		}

		int length	= sizeof(StreamMessage) - 3 * sizeof(uint8_t*);
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

		once = 0;
		uint32_t y_length = message->y_length;

		for (uint32_t i = 0; i < y_length; i += once)
		{
			once = send(Socket, message->y + i, y_length - i, MSG_CONFIRM);
			
			if (once == -1)
			{
				dzlog_error("errno: %d", errno);
				return false;
			}
		}

		once = 0;
		uint32_t u_length = message->u_length;

		for (uint32_t i = 0; i < u_length; i += once)
		{
			once = send(Socket, message->u + i, u_length - i, MSG_CONFIRM);
			
			if (once == -1)
			{
				dzlog_error("errno: %d", errno);
				return false;
			}
		}

		once = 0;
		uint32_t v_length = message->v_length;

		for (uint32_t i = 0; i < v_length; i += once)
		{
			once = send(Socket, message->v + i, v_length - i, MSG_CONFIRM);
			
			if (once == -1)
			{
				dzlog_error("errno: %d", errno);
				return false;
			}
		}

		dzlog_info("success for socket: %d", Socket);
		return true;
	}

	int SetStreamData(StreamMessage* message, uint8_t* value[3], uint32_t length[3])
	{
		{
			if (message == nullptr)
			{
				dzlog_error("message == nullptr");
				return -3;
			}

			if (message->y != nullptr)
			{
				dzlog_error("message->y != nullptr");
				return -4;
			}

			if (message->u != nullptr)
			{
				dzlog_error("message->u != nullptr");
				return -5;
			}

			if (message->v != nullptr)
			{
				dzlog_error("message->v != nullptr");
				return -6;
			}

			for (int i = 0; i < 3; ++i)
			{
				if (value[i] == nullptr)
				{
					dzlog_error("value[%d] == nullptr", i);
					return -i;
				}
			}
		}

		// 之所以校验 length != 0, 是因为确实可以没有那个分量
		if (length[0] != 0) message->y = new uint8_t[length[0]];
		if (length[1] != 0) message->u = new uint8_t[length[1]];
		if (length[2] != 0) message->v = new uint8_t[length[2]];

		memcpy(message->y, value[0], length[0]);
		memcpy(message->u, value[1], length[1]);
		memcpy(message->v, value[2], length[2]);

		message->y_length = length[0];
		message->u_length = length[1];
		message->v_length = length[2];

		message->length = 12 + length[0] + length[1] + length[2]; 

		return 0;
	}

	int ClearStreamMessage(StreamMessage* message)
	{
		if (message == nullptr)
		{
			dzlog_error("message == nullptr");
			return -1;
		}

		if (message->y != nullptr)
		{
			delete[] message->y;
			message->y = nullptr;
		}

		if (message->u != nullptr)
		{
			delete[] message->u;
			message->u = nullptr;
		}

		if (message->v != nullptr)
		{
			delete[] message->v;
			message->v = nullptr;
		}

		return 0;
	}

	int FullFromNet(StreamMessage* message, const connection& conn)
	{
		if (message == nullptr)
		{
			dzlog_error("message == nullptr");
			return -1;
		}

		uint32_t once = recv(conn.sockfd, &message->y_length, 12, MSG_WAITALL);
		if (once != 12)
		{
			dzlog_error("no enough data");
			return NEED_2_CLOSE_SOCKET_ERROR;
		}

		dzlog_info("get frame (y, u, v) == (%d, %d, %d)", message->y_length, message->u_length, message->v_length);

		message->y = new uint8_t[message->y_length];
		message->u = new uint8_t[message->u_length];
		message->v = new uint8_t[message->v_length];

		once = recv(conn.sockfd, message->y, message->y_length, MSG_WAITALL);
		if (once != message->y_length)
		{
			dzlog_error("not enough y data");
			ClearStreamMessage(message);
			return NEED_2_CLOSE_SOCKET_ERROR;
		}

		once = recv(conn.sockfd, message->u, message->u_length, MSG_WAITALL);
		if (once != message->u_length)
		{
			dzlog_error("not enough u data");
			ClearStreamMessage(message);
			return NEED_2_CLOSE_SOCKET_ERROR;
		}

		once = recv(conn.sockfd, message->v, message->v_length, MSG_WAITALL);
		if (once != message->v_length)
		{
			dzlog_error("not enough v data");
			ClearStreamMessage(message);
			return NEED_2_CLOSE_SOCKET_ERROR;
		}

		return 0;
	}
}

#endif 
