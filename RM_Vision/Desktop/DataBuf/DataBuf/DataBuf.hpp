/* kiko@idiospace.com 2020.01.20 */

#ifndef DataBuf_HPP
#define DataBuf_HPP

#include <mutex>
#include <chrono>
#include <vector>
#include <memory>
#include <exception>

namespace hnurm {


// Add time_stamp to the raw data type
template <typename RawDataType>
class Wrapped
{
public:
    RawDataType raw_data;
    short time_stamp;

public:
    Wrapped<RawDataType>& operator= (const Wrapped<RawDataType>& wrapped_data)
    {
        raw_data = wrapped_data.raw_data;
        time_stamp = wrapped_data.time_stamp;
        return *this;
    }
    Wrapped():
       time_stamp(clock())
    {}
    Wrapped(const RawDataType& data_, short time_stamp_=clock()):
        time_stamp(time_stamp_),
        raw_data(data_)
    {

    }
    ~Wrapped() = default;
    void wrap(RawDataType& raw_data_)
    {
        raw_data = raw_data_;
    }
private:
    static short clock_time;
    static short clock()
    {
        return ++clock_time;
    }
};

template <typename RawDataType>
short Wrapped<RawDataType>::clock_time = 0;


template <typename DataType>
class DataBuf
{
protected:
    using Ms = std::chrono::milliseconds;
    using validator = bool (*) (const DataType&);  // validate data func

public:
    DataBuf(int size=16);
    ~DataBuf() = default;
    bool get(DataType& data, validator v=nullptr);
    bool update(const DataType& data, validator v=nullptr);

private:
    std::vector<DataType> data_buf;
    std::vector<std::timed_mutex> mtx_buf;
    int head_idx, tail_idx;
    short latest_time_stamp;
};

template <class DataType>
DataBuf<DataType>::DataBuf(int size):
    head_idx(0),
    tail_idx(0),
    latest_time_stamp(0),
    data_buf(size),
    mtx_buf(size)
{

}

template <class DataType>
bool DataBuf<DataType>::get(DataType& data, validator v)
{
    try{
        const int cur_head_idx = head_idx;

        if (mtx_buf[cur_head_idx].try_lock_for(Ms(2)))  // lock succeed
        {
            // std::cout << "[get] lock succeed" << std::endl;
            if ((v == nullptr ? true: v(data_buf[cur_head_idx]))) // check data integrity if validator is given
            {
                // std::cout << "[get] data valid" << std::endl;
                if (data_buf[cur_head_idx].time_stamp != latest_time_stamp)  // data has not been used before
                {
                    latest_time_stamp = data_buf[cur_head_idx].time_stamp;
                    data = data_buf[cur_head_idx];
                    mtx_buf[cur_head_idx].unlock();
                    return true;
                }
            }
            mtx_buf[cur_head_idx].unlock();
        }
        else
        {
            // std::cout << "[get] failed to lock" << std::endl;
        }

        return false;
    } catch (std::exception& e)
    {
#ifdef DEBUG
        std::cerr << e.what() << std::endl;
#endif
    }
}



template <class DataType>
bool DataBuf<DataType>::update(const DataType& data, validator v)
{
    try{
        if (v!=nullptr && !v(data)) return false;

        const int new_head_idx = (head_idx + 1) % data_buf.size();

        if (mtx_buf[new_head_idx].try_lock_for(Ms(2)))  // lock succeed
        {
            data_buf[new_head_idx] = data;
            if (new_head_idx == tail_idx)
            {
                tail_idx = (tail_idx + 1) % data_buf.size();
            }
            head_idx = new_head_idx;
            mtx_buf[new_head_idx].unlock();
            return true;
         }
        return false;
    } catch (std::exception& e) {
#ifdef DEBUG
        std::cerr << e.what() << std::endl;
#endif
    }
}


}
#endif // DataBuf_H

