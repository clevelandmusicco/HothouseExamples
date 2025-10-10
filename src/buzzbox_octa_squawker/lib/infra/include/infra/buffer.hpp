/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_BUFFER_OCTOBER_15_2018)
#define INFRA_BUFFER_OCTOBER_15_2018

#include <infra/iterator_range.hpp>

namespace cycfi
{
   ////////////////////////////////////////////////////////////////////////////
   // audio_buffer is a platform independent representation of a multi
   // channel buffer. Hosts create audio buffers using a platform dependent
   // subclass of audio_buffer that supplies the concrete implementation of
   // the private get_channel_base member function.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   class audio_buffer
   {
   public:

      using sample_type = T;

                           audio_buffer(std::size_t num_channels, std::size_t size)
                            : _num_channels(num_channels)
                            , _size(size)
                           {}

      iterator_range<T*>   operator[](std::size_t channel) const;
      std::size_t          num_channels() const { return _num_channels; }
      std::size_t          size() const         { return _size; }

   private:

      virtual T*           get_channel_base(std::size_t channel) const = 0;

      std::size_t          _num_channels;
      std::size_t          _size;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline iterator_range<T*>
   audio_buffer<T>::operator[](std::size_t channel) const
   {
      T* start = get_channel_base(channel);
      return {start, start + _size};
   }
}

#endif
