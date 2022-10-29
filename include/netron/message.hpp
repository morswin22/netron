#pragma once

namespace netron
{

  // Message Header is sent at the start of every message
  template<typename T>
  struct message_header
  {
    T id{};
    uint32_t size = 0;
  };

  template<typename T>
  struct message
  {
    message_header<T> header{};
    std::vector<uint8_t> body;

    // Returns the size of the entire message in bytes
    size_t size() const
    {
      return body.size();
    }

    // Override for std::cout
    friend std::ostream& operator<<(std::ostream& os, const message<T>& msg)
    {
      os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
      return os;
    }

    // Push into the message buffer
    template<typename DataType> 
    friend message<T>& operator<<(message<T>& msg, const DataType& data);

    // Pop from the message buffer
    template<typename DataType> 
    friend message<T>& operator>>(message<T>& msg, DataType& data);

    // Push any trivially copyable data into the message buffer
    template<
      typename T,
      typename DataType, 
      typename std::enable_if<std::is_trivially_copyable<DataType>::value, bool>::type = true> 
    friend message<T>& operator<<(message<T>& msg, const DataType& data)
    {
      // Cache the current size of the vector
      size_t i = msg.body.size();

      // Resize the vector by the size of the data being pushed
      msg.body.resize(i + sizeof(DataType));

      // Copy the data into the newly allocated vector space
      std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

      // Recalculate the message size
      msg.header.size = msg.size();

      // Return the target message so it can be "chained"
      return msg;
    }

    // Pop any trivially copyable data from the message buffer
    template<
      typename T,
      typename DataType, 
      typename std::enable_if<std::is_trivially_copyable<DataType>::value, bool>::type = true> 
    friend message<T>& operator>>(message<T>& msg, DataType& data)
    {
      // Cache the current size of the vector
      size_t i = msg.body.size() - sizeof(DataType);

      // Copy the data from the vector into the user variable
      std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

      // Shrink the vector to remove the read bytes
      msg.body.resize(i);

      // Recalculate the message size
      msg.header.size = msg.size();

      // Return the target message so it can be "chained"
      return msg;
    }

    // Push a contiguous container with trivial data into the message buffer
    template<
      typename T,
      typename ContainerType,
      typename DataType = std::remove_pointer<decltype(std::declval<ContainerType>().data())>::type,
      typename std::enable_if<std::is_trivially_copyable<DataType>::value, bool>::type = true>
    friend message<T>& operator<<(message<T>& msg, const ContainerType& data)
    {
      // Copy the container data into the vector
      const size_t data_size = data.size();
      msg.body.resize(msg.body.size() + data_size * sizeof(DataType));
      std::memcpy(msg.body.data() + msg.body.size() - data_size * sizeof(DataType), data.data(), data_size * sizeof(DataType));

      // Copy the container size into the vector
      msg << data_size;

      // Recalculate the message size
      msg.header.size = msg.size();

      // Return the target message so it can be "chained"
      return msg;
    }

    // Pop a contiguous container with trivial data from the message buffer
    template<
      typename T,
      typename ContainerType,
      typename DataType = std::remove_pointer<decltype(std::declval<ContainerType>().data())>::type,
      typename std::enable_if<std::is_trivial<DataType>::value, bool>::type = true>
    friend message<T>& operator>>(message<T>& msg, ContainerType& data)
    {
      // Get the size of the container being popped
      size_t container_size;
      msg >> container_size;

      // Resize the container to the correct size
      data.resize(container_size);

      // Copy the container data from the vector
      std::memcpy(const_cast<std::remove_const<DataType>::type*>(data.data()), msg.body.data() + msg.body.size() - container_size * sizeof(DataType), container_size * sizeof(DataType));
      msg.body.resize(msg.body.size() - container_size * sizeof(DataType));

      // Recalculate the message size
      msg.header.size = msg.size();

      // Return the target message so it can be "chained"
      return msg;
    }

    // Push a container into the message buffer
    template<
      typename T,
      typename ContainerType,
      typename DataType = std::iterator_traits<decltype(std::declval<ContainerType>().begin())>::value_type,
      typename std::enable_if<!std::is_trivially_copyable<DataType>{}, bool>::type = true>
    friend message<T>& operator<<(message<T>& msg, const ContainerType& data)
    {
      // Copy the container data into the vector
      for (auto it = data.begin(); it != data.end(); ++it)
        msg << *it;

      // Copy the container size into the vector
      msg << data.size();

      // Recalculate the message size
      msg.header.size = msg.size();

      // Return the target message so it can be "chained"
      return msg;
    }

    // Pop a container from the message buffer
    template<
      typename T,
      typename ContainerType,
      typename DataType = std::iterator_traits<decltype(std::declval<ContainerType>().begin())>::value_type,
      typename std::enable_if<std::is_default_constructible<DataType>{} && !std::is_trivial<DataType>{}, bool>::type = true>
    friend message<T>& operator>>(message<T>& msg, ContainerType& data)
    {
      // Get the size of the container being popped
      size_t container_size;
      msg >> container_size;

      // Resize the container to the correct size
      data.resize(container_size);

      // Copy the container data from the vector
      for (auto it = data.rbegin(); it != data.rend(); ++it)
        msg >> *it;

      // Recalculate the message size
      msg.header.size = msg.size();

      // Return the target message so it can be "chained"
      return msg;
    }
  };

  // Forward declaration of connection class
  template<typename T>
  class connection;

  template<typename T>
  struct owned_message
  {
    std::shared_ptr<connection<T>> remote = nullptr;
    message<T> msg;

    // Override for std::cout
    friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
    {
      os << msg.msg;
      return os;
    }
  };

}
