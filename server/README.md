# Server

This server checks if the user exists in the database by checking if the pin or rfid exist. If the user exists, the server will return `{authenticated: true, message: "Welcome [user]"}`. If the user does not exist, the server will return `{authenticated: false}`.

Note: this is a very insecure way of authenticating users. This is only for demonstration purposes.

## Installation

1. install [node.js](https://nodejs.org/en/download/) (can just use the newest LTS version)

2. change directory into the server 
```bash
cd server
```

3. install dependencies
```bash
npm install
```

## Usage
1. Figure out your IP address. You can do this by running `ipconfig` on Windows or `ifconfig` on Linux/MacOS. You can also use the IP address of the computer you are running the server on.

2. Change the `ip` variable `apiUrl` in `RFID.ino` to your IP address. It should look something like this: `http://127.0.0.22:8080`


3. Start the server

```bash
npm start
```

This starts the server on port 8080. You can change the port in `main.js`. The server will also live reload when you make changes to the code.

## Database
To clear the database, just start and stop the server. The database is stored in memory, so it will be cleared when the server stops.

## API
The server has two endpoints: `/pin/:pin` and `/rfid:rfid`. Both endpoints accept a GET request. The server will return a JSON response with the `authenticated` key and a `message` key if the user exists. The `message` key will contain the user's name.