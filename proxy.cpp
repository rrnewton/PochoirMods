// an example showing how to design Proxy class
//
// by Dahua Lin
//

#include <iostream>

using namespace std;

// do whatever you want for setting
template<typename T>
inline void trace_setter(T v)
{
    std::cout << "setting value " << v << std::endl;
}


template<typename T>
class ValueProxy
{
public:
    explicit ValueProxy(T& v) : m_value(v)
    {
    }

    operator T() const  // the implicit conversion makes a proxy just like the value itself
    {
        cout << "read VPArray" << endl;
	return m_value;
    }

    ValueProxy<T>& operator = (const T& rhs)
    {
	trace_setter(rhs);
        cout << "write VPArray" << endl;
	m_value = rhs;
    }

    T value() const
    {
	return m_value;
    }

    T& ref()
    {
	return m_value;
    }

private:
    T& m_value;
};


template<typename T>
class VPArray
{
public:
    VPArray(int n, const T& initvalue)
    {
	m_num = n;
	m_data = new T[n];
	for (int i = 0; i < n; ++i) 
	    m_data[i] = initvalue;	
    }

    ~VPArray()
    {
	delete[] m_data;
    }

    T operator() (int i) const
    {
	return m_data[i];
    }

    ValueProxy<T> operator() (int i) 
    {
	return ValueProxy<T>(m_data[i]);
    }

private:
    // disable copying
    // (For simplicity, I just don't want to implement the copy constructor and assignment)
    
    VPArray(const VPArray& );
    VPArray& operator = (const VPArray& );

private:
    int m_num;
    T *m_data;
};


int main(int argc, char *argv[])
{
    VPArray<int> a(3, 0);

    a(0) = -1;
    a(1) = -2;
    a(2) = -3;

    std::cout << a(2) << std::endl;  // output 3

    a(2) = a(1) + 10;   // note here that a(2) has been changed to 12

    std::cout << a(2) << std::endl;  // output 12

    return 0;
}


