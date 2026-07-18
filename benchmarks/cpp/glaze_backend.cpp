#include "benchmark_runtime.hpp"

#include <glaze/cbor.hpp>
#include <glaze/glaze.hpp>

namespace serdde_benchmark {

struct QuitEnvelope {
  Quit value;

  struct glaze {
    using T = QuitEnvelope;
    static constexpr auto value = glz::object("Quit", &T::value);
  };
};

struct KeyDownEnvelope {
  KeyDown value;

  struct glaze {
    using T = KeyDownEnvelope;
    static constexpr auto value = glz::object("KeyDown", &T::value);
  };
};

struct MouseMoveEnvelope {
  MouseMove value;

  struct glaze {
    using T = MouseMoveEnvelope;
    static constexpr auto value = glz::object("MouseMove", &T::value);
  };
};

using GlazeEvent =
    std::variant<QuitEnvelope, KeyDownEnvelope, MouseMoveEnvelope>;

struct GlazeEventRecord {
  std::uint64_t sequence{};
  GlazeEvent event;
};

GlazeEventRecord to_glaze(const EventRecord &record) {
  GlazeEventRecord converted;
  converted.sequence = record.sequence;
  converted.event = std::visit(
      [](const auto &event) -> GlazeEvent {
        using T = std::decay_t<decltype(event)>;
        if constexpr (std::is_same_v<T, Quit>) {
          return QuitEnvelope{event};
        } else if constexpr (std::is_same_v<T, KeyDown>) {
          return KeyDownEnvelope{event};
        } else {
          return MouseMoveEnvelope{event};
        }
      },
      record.event);
  return converted;
}

EventRecord from_glaze(const GlazeEventRecord &record) {
  EventRecord converted;
  converted.sequence = record.sequence;
  converted.event = std::visit(
      [](const auto &event) -> BenchEvent {
        using T = std::decay_t<decltype(event)>;
        if constexpr (std::is_same_v<T, QuitEnvelope>) {
          return event.value;
        } else if constexpr (std::is_same_v<T, KeyDownEnvelope>) {
          return event.value;
        } else {
          return event.value;
        }
      },
      record.event);
  return converted;
}

struct GlazeBackend {
  static const char *name() { return "glaze"; }
  static bool supports(const std::string &format) {
    return format == "json" || format == "cbor";
  }

  template <typename T>
  static bool encode(const T &value, const std::string &format,
                     std::string &output) {
    if constexpr (std::is_same_v<T, EventRecord>) {
      const GlazeEventRecord converted = to_glaze(value);
      return encode_value(converted, format, output);
    } else {
      return encode_value(value, format, output);
    }
  }

  template <typename T>
  static bool decode(const std::string &source, const std::string &format,
                     T &output) {
    if constexpr (std::is_same_v<T, EventRecord>) {
      GlazeEventRecord converted;
      if (!decode_value(source, format, converted)) {
        return false;
      }
      output = from_glaze(converted);
      return true;
    } else {
      return decode_value(source, format, output);
    }
  }

private:
  template <typename T>
  static bool encode_value(const T &value, const std::string &format,
                           std::string &output) {
    const glz::error_ctx error = format == "cbor"
                                     ? glz::write_cbor(value, output)
                                     : glz::write_json(value, output);
    return !error;
  }

  template <typename T>
  static bool decode_value(const std::string &source, const std::string &format,
                           T &output) {
    const glz::error_ctx error = format == "cbor"
                                     ? glz::read_cbor(output, source)
                                     : glz::read_json(output, source);
    return !error;
  }
};

} // namespace serdde_benchmark

int main() {
  return serdde_benchmark::benchmark_main<serdde_benchmark::GlazeBackend>();
}
