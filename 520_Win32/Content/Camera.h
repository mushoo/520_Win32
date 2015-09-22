#pragma once

#include "pch.h"
#include "StepTimer.h"
#include "winuser.h"

using namespace DirectX;

namespace _520
{
	class Camera
	{
	public:
		Camera()
		{
			m_speed = 2.0f;
			m_sensitivity = 0.005f;

			m_forward = m_back = m_left = m_right = m_pointer = false;
			m_position = XMVectorSet(0.405847669, 0.105964065, -3.34974909, 0);
			m_yaw = 0.110001020;
			m_pitch = -0.434999615;
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
		}

		void OnKeyUp(int virtualKey)
		{
			if (virtualKey == 'W') m_forward = false;
			if (virtualKey == 'S') m_back = false;
			if (virtualKey == 'A') m_left = false;
			if (virtualKey == 'D') m_right = false;
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

			viewMatrix = XMMatrixLookToRH(m_position, forwardVec, upVec);
		}

		XMMATRIX GetView()
		{
			return viewMatrix;
		}

	private:
		bool m_forward, m_back, m_left, m_right, m_pointer;
		float m_speed, m_sensitivity;
		int m_pointerX, m_pointerY;
		XMVECTOR m_position;
		XMMATRIX viewMatrix;
		float m_yaw, m_pitch;
	};
}