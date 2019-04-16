#ifndef SAFE_LIST__
#define SAFE_LIST__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <list>

using std::list;

#define SEM_INIT(sem, sz) do {\
	int ret =  -1;\
	ret = sem_init(&sem, 0, sz);\
	if(ret != 0 ){\
		perror("SafeList sem init error");\
		exit(errno);\
	}\
}while(0)

#define SEM_WAIT(sem) do {\
	int ret =  -1;\
	ret = sem_wait(&sem);\
	if(ret != 0 ){\
		perror("SafeList sem wait error");\
		exit(errno);\
	}\
}while(0)

#define SEM_POST(sem) do {\
	int ret =  -1;\
	ret = sem_post(&sem);\
	if(ret != 0 ){\
		perror("SafeList sem post error");\
		exit(errno);\
	}\
}while(0)

#define MUTEX_INIT(mtx) do {\
	int ret =  -1;\
	ret = pthread_mutex_init(&mtx, 0);\
	if(ret != 0 ){\
		perror("SafeList mtx init error");\
		exit(errno);\
	}\
}while(0)

#define MUTEX_LOCK(mtx) do {\
	int ret =  -1;\
	ret = pthread_mutex_lock(&mtx);\
	if(ret != 0 ){\
		perror("SafeList mtx lock error");\
		exit(errno);\
	}\
}while(0)

#define MUTEX_UNLOCK(mtx) do {\
	int ret =  -1;\
	ret = pthread_mutex_unlock(&mtx);\
	if(ret != 0 ){\
		perror("SafeList mtx unlock error");\
		exit(errno);\
	}\
}while(0)

template<typename T>
class SafeList{
public:
	SafeList();
	SafeList(size_t sz);
	~SafeList();
	T* pop();
	size_t size();
	void push(T*);
private:
	SafeList(SafeList& ref){};
	void Init(size_t);
private:
	std::list<T*> 	list_;
	size_t 			list_sz_;
	sem_t			sem_;
#ifdef SPIN_LOCK_
	pthread_spinlock_t spinlock_;
#else
	pthread_mutex_t	mtx_;
#endif
};

template<typename T>
void SafeList<T>::Init(size_t sz)
{
	SEM_INIT(sem_, 0);
#ifdef SPIN_LOCK_
	pthread_spin_init(&spinlock_, PTHREAD_PROCESS_PRIVATE);
#else
	MUTEX_INIT(mtx_);
	MUTEX_LOCK(mtx_);
	MUTEX_UNLOCK(mtx_);
#endif
	T* pT = NULL;

	for(size_t i = 0; i < sz; i++){
		pT = new T;
 		if(pT == NULL){
			perror("New T error");
			exit(errno);
		}	
		list_.push_back(pT);
		SEM_POST(sem_);
	}
}

template<typename T>
SafeList<T>::SafeList()
{
	list_sz_ = 1000;
	Init(list_sz_);
}

template<typename T>
SafeList<T>::SafeList(size_t sz)
{
	list_sz_ = sz;
	Init(list_sz_);
}
template<typename T>
SafeList<T>::~SafeList()
{
	if(list_sz_ > 0){
		typename std::list<T*>::iterator iter = list_.begin();
		for(; iter != list_.end(); iter++){
			T *tmp = *iter;
			*iter = NULL;
			delete tmp;
		}
	}
	list_.resize(0);
	sem_destroy(&sem_);
#ifdef SPIN_LOCK_
	pthread_spin_destroy(&spinlock_);
#else
	pthread_mutex_destroy(&mtx_);
#endif	
}

template<typename T>
T* SafeList<T>::pop()
{
	T *tmp = NULL;
	SEM_WAIT(sem_);
#ifdef SPIN_LOCK_
	pthread_spin_lock(&spinlock_);
#else
	MUTEX_LOCK(mtx_);
#endif	
	tmp = list_.front();
	list_.pop_front();
#ifdef SPIN_LOCK_
	pthread_spin_unlock(&spinlock_);
#else
	MUTEX_UNLOCK(mtx_);
#endif
	return tmp;
}
template<typename T>
size_t SafeList<T>::size()
{
	return list_sz_;
}
template<typename T>
void SafeList<T>::push(T *pdata)
{
	T *tmp = NULL;
#ifdef SPIN_LOCK_
	pthread_spin_lock(&spinlock_);
#else
	MUTEX_LOCK(mtx_);
#endif	
	list_.push_back(pdata);
#ifdef SPIN_LOCK_
	pthread_spin_unlock(&spinlock_);
#else
	MUTEX_UNLOCK(mtx_);
#endif
	SEM_POST(sem_);
}
#endif /* SAFE_LIST__ */
