#pragma once


template<class T>
class Singleton
{
protected:
	static T *_instance;

public:
	Singleton() { }
	virtual ~Singleton() { }

	inline static T *Get()
	{
		if (_instance == nullptr)
			_instance = new T;
		return _instance;
	}

	inline static void Destroy()
	{
		if (_instance != nullptr)
		{
			delete _instance;
			_instance = nullptr;
		}
	}
};

template <class T>
T* Singleton<T>::_instance = nullptr;
