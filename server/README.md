# Server

This server checks if the user exists in the database by checking if the pin or rfid exist. If the user exists, the server will return `{authenticated: true, message: "Welcome [user]"}`. If the user does not exist, the server will return `{authenticated: false}`.

Note: this is a very insecure way of authenticating users. This is only for demonstration purposes.

## Installation

```bash
npm install
```

## Usage

```bash
npm start
```

This starts the server on port 8080. You can change the port in `main.js`. The server will also live reload when you make changes to the code.

## API
The server has two endpoints: `/pin/:pin` and `/rfid:rfid`. Both endpoints accept a GET request. The server will return a JSON response with the `authenticated` key and a `message` key if the user exists. The `message` key will contain the user's name.