#pragma once

#include <exception>
#include <thread>
#include <queue>
#include <vector>
#include <functional>
#include <type_traits>
#include <utility>

/// <summary>
/// Function used to execute an action on the selected resource once it is loaded.
/// </summary>
/// <typeparam name="T">The type of resource to use.</typeparam>
template<class T>
using Action = void(*)(T* const);
/// <summary>
/// Function used to execute an action for a certain exception if one gets caught.
/// </summary>
using ErrorAction = void(*)(const std::exception&);

template<class T>
concept _LoadingFuncReturnT = !std::is_void<T>::value;

template<_LoadingFuncReturnT LoadingFuncReturnT, class... LoadingFuncParamT>
class Task
{
public:
	using LoadingFunc = LoadingFuncReturnT(*)(LoadingFuncParamT...);

	Task(LoadingFunc func, LoadingFuncParamT&&... params)
		: m_Thread(
			[&]()
			{
				m_Result = func(std::forward<LoadingFuncParamT&&>(params)...);

				while (!m_Actions.empty())
				{
					m_Actions.front()(m_Result);
					m_Actions.pop();
				}
			}
		)
	{
	}

	void Join()
	{
		m_Thread.join();
	}

	bool Joinable() const
	{
		return m_Thread.joinable();
	}

	bool HasResult() const
	{
		return m_Result != nullptr;
	}

	LoadingFuncReturnT* GetResult() const
	{
		return m_Result;
	}
	
	Task& Then(Action<LoadingFuncReturnT> action)
	{
		m_Actions.push(action);
		return *this;
	}

	/// <summary>
	/// Called if an exception occurs during an Action.
	/// </summary>
	/// <param name="action">The Action to execute if an exception occurs.</param>
	/// <returns></returns>
	Task& Catch(ErrorAction action)
	{
		m_ErrorActions.push_back(action);
		return *this;
	}

private:
	std::thread m_Thread;

	std::queue<Action<LoadingFuncReturnT>> m_Actions;
	std::vector<ErrorAction> m_ErrorActions;

	LoadingFuncReturnT* m_Result = nullptr;
};
