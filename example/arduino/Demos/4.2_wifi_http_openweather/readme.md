# OpenWeatherMap Demo

This example fetches weather information from the OpenWeatherMap API and renders
the current conditions on the 4.2" e-paper display. The sketch shows the current
temperature, daily maximum, humidity and sunrise/sunset times.

Main functions:
- `js_analysis()` retrieves and parses the JSON forecast data.
- `updateDateTimeCity()` updates the time and location string.
- `UI_weather_forecast()` draws the weather screen.

Copy `secrets_example.h` to `secrets.h` and fill in your WiFi credentials and API
key before compiling.

