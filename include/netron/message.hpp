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

    // Push any trivially copyable data into the message buffer
    template<typename DataType>
    friend message<T>& operator<<(message<T>& msg, const DataType& data)
    {
      // Check that the type of the data being pushed is trivially copyable
      static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

      // Cache the current size of the vector
      size_t i = msg.body.size();

      // Resize the vector by the size of the data being pushed
      msg.body.resize(msg.body.size() + sizeof(DataType));

      // Copy the data into the newly allocated vector space
      std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

      // Recalculate the message size
      msg.header.size = msg.size();

      // Return the target message so it can be "chained"
      return msg;
    }

    // Pop any trivially copyable data from the message buffer
    template<typename DataType>
    friend message<T>& operator>>(message<T>& msg, DataType& data)
    {
      // Check that the type of the data being pushed is trivially copyable
      static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

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