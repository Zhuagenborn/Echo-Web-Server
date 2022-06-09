# Logger

## Overview

```mermaid
classDiagram

class Level {
    <<enumeration>>
    Debug
    Info
    Warn
    Error
    Fatal
}

class Event {
    Level level
    source_location location
    time_point date
    string message
}

Event --> Level

class Formatter {
    Format(Logger, Event) string
}

class Field {
    <<interface>>
    Format(Logger, Event) string
}

Formatter *-- Field
Field ..> Event

class Appender {
    <<abstract>>
    Log(Logger, Event)*
}

Appender --> Formatter

class StdOutAppender
Appender <|-- StdOutAppender

class FileAppender
Appender <|-- FileAppender

class Logger {
    Log(Event)
}

Logger o-- Event
Logger o-- Appender
Logger --> Level

class Manager {
    FindLogger(name, Level) Logger
}

Manager o-- Logger

class ThreadId
Field <|.. ThreadId

class DateTime
Field <|.. DateTime

class FileName
Field <|.. FileName

class LineNum
Field <|.. LineNum

class Message
Field <|.. Message

class LoggerName
Field <|.. LoggerName

class RawString
Field <|.. RawString
```

## Interactions

### Event Logging

```mermaid
sequenceDiagram

participant Manager
participant Logger
participant Appender
participant Formatter
participant Field

Manager ->> Logger: FindLogger

loop For every appender
Logger ->> Appender: Log
Appender ->> Formatter: Log

loop For every field
Formatter ->> Field: Format
Field -->> Formatter: Message
end

Formatter -->> Appender: Message
end
```

## Configuration

The logger configuration can be defined using `YAML`. Here is a sample.

```yaml
# The root tag.
loggers:
  # A logger's name.
  - name: root
    # The current logger's level which can be "debug", "info", "warn", "error" or "fatal".
    level: info
    # The current logger's event capacity.
    # If it is zero, the logger will be synchronous, otherwise asynchronous.
    capacity: 50
    # The current logger's appenders.
    appenders:
      # This logger has two appender.
      # A appender's type which can be "stdout" or "file".
      - type: stdout
      - type: file
        # A file appender needs a file name.
        file: log.txt
  - name: system
    level: debug
    # A custom format.
    formatter: "%d%T%m%n"
    appenders:
      # This logger has one appender.
      - type: stdout
```

The logger supports two kinds of appenders:

- The *Standard Output Appender*

  It writes events to the standard output stream.

  ```yaml
  appenders:
    - type: stdout
  ```

- The *File Appender*

  It writes events to a local file.

  ```yaml
  appenders:
    - type: file
      file: log.txt
  ```