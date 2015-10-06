#pragma once

#include "pch.h"
#include "StepTimer.h"
#include "winuser.h"
#include "..\DeviceResources.h"
#include "DirectXHelper.h"
#include <map>
#include <tuple>
#include <utility>

using namespace DirectX;

namespace _520
{
	class Profiler
	{
	public:
		Profiler(std::shared_ptr<DX::DeviceResources> deviceResources)
		{
			m_deviceResources = deviceResources;
			auto device = m_deviceResources->GetD3DDevice();
			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			desc.MiscFlags = 0;
			DX::ThrowIfFailed(device->CreateQuery(&desc, &m_disjointQuery));
			desc.Query = D3D11_QUERY_TIMESTAMP;
			DX::ThrowIfFailed(device->CreateQuery(&desc, &m_beginFrame));
			DX::ThrowIfFailed(device->CreateQuery(&desc, &m_endFrame));
		}
		void beginFrame()
		{
			auto context = m_deviceResources->GetD3DDeviceContext();
			context->Begin(m_disjointQuery);
			context->End(m_beginFrame);
			m_queriesInOrder.reserve(m_queries.size());
			m_queriesInOrder.clear();
			QueryPerformanceCounter((LARGE_INTEGER *)&CPUStart);
		}
		void endFrame()
		{
			auto context = m_deviceResources->GetD3DDeviceContext();
			context->End(m_endFrame);
			context->End(m_disjointQuery);
			QueryPerformanceCounter((LARGE_INTEGER *)&CPUEnd);
		}
		void nextTime(std::string name)
		{
			auto context = m_deviceResources->GetD3DDeviceContext();
			auto iterator = m_queries.find(name);
			if (iterator == m_queries.end())
				addQuery(name);
			ID3D11Query *query = m_queries.at(name);
			context->End(query);
			UINT64 CPUTime;
			QueryPerformanceCounter((LARGE_INTEGER *)&CPUTime);
			m_queriesInOrder.push_back(std::make_tuple(name, query, CPUTime));
		}
		std::shared_ptr<std::vector<std::tuple<std::string, float, float>>> getTimings()
		{
			auto context = m_deviceResources->GetD3DDeviceContext();
			auto result = std::make_shared<std::vector<std::tuple<std::string, float, float>>>();
			// Basically just wait for the data to be available.
			while (context->GetData(m_disjointQuery, NULL, 0, 0) == S_FALSE)
			{
				Sleep(1);
			}

			D3D10_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
			context->GetData(m_disjointQuery, &disjointData, sizeof(disjointData), 0);
			if (disjointData.Disjoint)
				return result;

			UINT64 freqCPU;
			QueryPerformanceFrequency((LARGE_INTEGER *)&freqCPU);

			UINT64 ticks;
			context->GetData(m_beginFrame, &ticks, sizeof(ticks), 0);
			UINT64 firstCPU = CPUStart;
			UINT64 previousCPU = firstCPU;
			UINT64 first = ticks;
			UINT64 previous = first;
			for (int i = 0; i < m_queriesInOrder.size(); i++)
			{
				UINT64 currentCPU = std::get<2>(m_queriesInOrder[i]);
				context->GetData(std::get<1>(m_queriesInOrder[i]), &ticks, sizeof(ticks), 0);
				UINT64 current = ticks;
				result->push_back(std::make_tuple(
					std::get<0>(m_queriesInOrder[i]),
					(current - previous) / (float)disjointData.Frequency * 1000.0f,
					(currentCPU - previousCPU) / (float)freqCPU * 1000.0f
				));
				previous = current;
				previousCPU = currentCPU;
			}
			context->GetData(m_endFrame, &ticks, sizeof(ticks), 0);
			UINT64 last = ticks;
			result->push_back(std::make_tuple(
				"TOTAL TIME",
				(last - first) / (float)disjointData.Frequency * 1000.0f,
				(CPUEnd - CPUStart) / (float)freqCPU * 1000.0f
			));
			return result;
		}

	private:
		void addQuery(std::string name)
		{
			auto device = m_deviceResources->GetD3DDevice();
			ID3D11Query *query;
			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP;
			desc.MiscFlags = 0;
			DX::ThrowIfFailed(device->CreateQuery(&desc, &query));
			m_queries[name] = query;
		}
		ID3D11Query *m_disjointQuery, *m_beginFrame, *m_endFrame;
		UINT64 CPUStart, CPUEnd;
		std::map<std::string, ID3D11Query *> m_queries;
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::vector<std::tuple<std::string, ID3D11Query *, UINT64>> m_queriesInOrder;
	};
}