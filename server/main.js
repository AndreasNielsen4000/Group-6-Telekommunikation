const express = require("express");
const app = express();
const port = 8080;

// mock database
let usersDatabase = [{
        name: "John Doe",
        pin: 1234,
        rfid: ["1234567890"]
    },
    {
        name: "Jane Doe",
        pin: 4321,
        rfid: ["0987654321"]
    }
];

// middleware

// check if pin exists in the database and returns `authenticated: false` if not
const checkPin = (req, res, next) => {
    const pin = req.params.pin;
    const user = usersDatabase.find(user => user.pin === parseInt(pin));
    if (user) {
        req.user = user;
        next();
    } else {
        // unauthorized
        res.status(401).json({
            authenticated: false
        });
    }
};

// check if rfid exists in the database and returns `authenticated: false` if not
const checkRfid = (req, res, next) => {
    const rfid = req.params.rfid;
    const user = usersDatabase.find(user => user.rfid.includes(rfid));
    if (user) {
        req.user = user;
        next();
    } else {
        // unauthorized
        res.status(401).json({
            authenticated: false
        });
    }
};

// routes
app.get("/pin/:pin", checkPin, (req, res) => {
    const user = req.user;

    res.json({
        authenticated: true,
        message: "Welcome " + user.name,
    });

});

app.get("/rfid/:rfid", checkRfid, (req, res) => {
    const user = req.user;

    res.json({
        authenticated: true,
        message: "Welcome " + user.name,
    });
});


// start server
app.listen(port, () => {
    console.log(`Example app listening on port ${port}`)
})