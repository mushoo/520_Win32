#pragma once

#include "pch.h"
#include "StepTimer.h"
#include "winuser.h"
#include <iostream>
#include <fstream>
#include <tuple>

using namespace DirectX;
using namespace std;

#define RECORDPATH false
#define PLAYPATH true
#define FRAMESPERWAYPOINT 100

namespace _520
{
	class Camera
	{
	public:
		Camera()
		{
			m_speed = 2.0f;
			m_sensitivity = 0.005f;
			m_frameCount = 0;
			m_firstRunComplete = false;

			if (PLAYPATH)
			{
				ifstream ifile("Camera_path.txt");
				XMFLOAT4 pos;
				float angle[2];
				while (ifile >> pos.x >> pos.y >> pos.z >> pos.w >> angle[0] >> angle[1])
				{
					XMVECTOR vecPos = XMLoadFloat4(&pos);
					auto tup = make_tuple(vecPos, angle[0], angle[1]);
					waypoints.push_back(tup);
				}
				ifile.close();
			}

			m_forward = m_back = m_left = m_right = m_pointer = m_recordPos = false;
			m_position = XMVectorSet(1.11859620, 0.741184413, 0.141090140, 0);
			m_yaw = 1.74999952;
			m_pitch = 0.369999856;
			//m_position = XMVectorSet(0, 0, 0, 0);
			//m_yaw = m_pitch = 0;
		}

		void OnPointerPressed(int xCoord, int yCoord)
		{
			m_pointer = true;
			m_pointerX = xCoord;
			m_pointerY = yCoord;
		}

		void OnPointerMoved(int xCoord, int yCoord)
		{
			if (!m_pointer) return; // Make sure we're clicking.

			float deltaX = xCoord - m_pointerX;
			float deltaY = yCoord - m_pointerY;
			m_yaw += deltaX * m_sensitivity;
			m_pitch += deltaY * m_sensitivity;

			m_pitch = max(-XM_PIDIV2, m_pitch);
			m_pitch = min(XM_PIDIV2, m_pitch);

			m_pointerX = xCoord;
			m_pointerY = yCoord;
		}

		void OnPointerReleased()
		{
			m_pointer = false;
		}

		void OnKeyDown(int virtualKey)
		{
			if (virtualKey == 'W') m_forward = true;
			if (virtualKey == 'S') m_back = true;
			if (virtualKey == 'A') m_left = true;
			if (virtualKey == 'D') m_right = true;
			if (virtualKey == 'Q') m_recordPos = true;
		}

		void OnKeyUp(int virtualKey)
		{
			if (virtualKey == 'W') m_forward = false;
			if (virtualKey == 'S') m_back = false;
			if (virtualKey == 'A') m_left = false;
			if (virtualKey == 'D') m_right = false;
			if (virtualKey == 'Q') m_recordPos = false;
		}

		std::string toString(XMVECTOR vec)
		{
			std::string str = "";
			str += std::to_string(vec.m128_f32[0]) + " " +
				std::to_string(vec.m128_f32[1]) + " " +
				std::to_string(vec.m128_f32[2]) + " " +
				std::to_string(vec.m128_f32[3]);
			return str;
		}

		bool doneFirstRun()
		{
			return m_firstRunComplete;
		}

		void Update(DX::StepTimer const& timer)
		{
			XMVECTOR forwardVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			XMVECTOR rightVec = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
			XMMATRIX camTransMat = XMMatrixRotationRollPitchYaw(m_pitch, -m_yaw, 0.0f);
			forwardVec = XMVector3TransformCoord(forwardVec, camTransMat);
			rightVec = XMVector3TransformCoord(rightVec, camTransMat);
			XMVECTOR upVec = XMVector3Cross(forwardVec, rightVec);

			double delta = timer.GetElapsedSeconds();
			XMVECTOR direction = XMVectorSet(0, 0, 0, 0);
			if (m_left) direction += rightVec;
			if (m_right) direction += -rightVec;
			if (m_back) direction += -forwardVec;
			if (m_forward) direction += forwardVec;
			m_position += direction * m_speed * delta;

			if (PLAYPATH)
			{
				float t = (m_frameCount % FRAMESPERWAYPOINT) / (float)FRAMESPERWAYPOINT;
				tuple<XMVECTOR, float, float> start = waypoints[m_frameCount / FRAMESPERWAYPOINT];
				tuple<XMVECTOR, float, float> end;
				if (m_frameCount / FRAMESPERWAYPOINT >= waypoints.size() - 1)
					end = waypoints[0];
				else end = waypoints[m_frameCount / FRAMESPERWAYPOINT + 1];
				m_position = XMVectorLerp(get<0>(start), get<0>(end), t);
				if (get<1>(end) -get<1>(start) > g_XMPi.f[0])
					get<1>(start) += 2 * g_XMPi.f[0];
				if (get<2>(end) -get<2>(start) > g_XMPi.f[0])
					get<2>(start) += 2 * g_XMPi.f[0];
				m_pitch = get<1>(start) +(get<1>(end) -get<1>(start)) * t;
				m_yaw = get<2>(start) +(get<2>(end) -get<2>(start)) * t;

				forwardVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
				rightVec = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
				camTransMat = XMMatrixRotationRollPitchYaw(m_pitch, -m_yaw, 0.0f);
				forwardVec = XMVector3TransformCoord(forwardVec, camTransMat);
				rightVec = XMVector3TransformCoord(rightVec, camTransMat);
				upVec = XMVector3Cross(forwardVec, rightVec);
			}

			viewMatrix = XMMatrixLookToRH(m_position, forwardVec, upVec);

			// Record position:
			if (RECORDPATH && m_recordPos)
			{
				if (!m_ofile.is_open())
					m_ofile.open("Camera_path.txt");
				m_ofile << toString(m_position) << "\n";
				m_ofile << to_string(m_pitch) + " " + to_string(m_yaw) << std::endl;
				m_recordPos = false;
			}
			m_frameCount++;
			if (m_frameCount / FRAMESPERWAYPOINT >= waypoints.size()){
				m_frameCount = 0;
				m_firstRunComplete = true;
			}
		}

		XMMATRIX GetView()
		{
			return viewMatrix;
		}

	private:
		bool m_forward, m_back, m_left, m_right, m_pointer, m_recordPos;
		ofstream m_ofile;
		vector<tuple<XMVECTOR, float, float>> waypoints;
		int m_frameCount;
		bool m_firstRunComplete;
		float m_speed, m_sensitivity;
		int m_pointerX, m_pointerY;
		XMVECTOR m_position;
		XMMATRIX viewMatrix;
		float m_yaw, m_pitch;
	};
}