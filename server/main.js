const express = require("express");
const app = express();
const port = 8080;

// mock database
let usersDatabase = [{
        id: 0,
        pin: 1234,
        rfid: ["1234567890"]
    },
    {
        id: 1,
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
        message: "Welcome user " + user.id,
    });

});

app.get("/rfid/:rfid", checkRfid, (req, res) => {
    const user = req.user;

    res.json({
        authenticated: true,
        message: "Welcome user " + user.id,
    });
});

// Get database
app.get("/database", (req, res) => {
    res.json(usersDatabase);
});

// Update pin for user
app.put("/pin/:id/:pin", (req, res) => {
    const id = req.params.id;
    const pin = req.params.pin;

    // TODO: check if database goes to 5
    if(id <= 0 || id > 5) {
        usersDatabase[id].pin = pin;
        res.json({
            success: true,
        });
    } else {
        res.status(401).json({
            success: false,
        });
    }

});

// Update rfid for user
app.put("/rfid/:id/:rfid", (req, res) => {
    const id = req.params.id;
    const rfid = req.params.rfid;

    if(id <= 0 || id > 5) {
        usersDatabase[id].rfid = rfid;
        res.json({
            success: true,
        });
    } else {
        res.status(401).json({
            success: false,
        });
    }
});


// start server
app.listen(port, () => {
    console.log(`Example app listening on port ${port}`)
})

