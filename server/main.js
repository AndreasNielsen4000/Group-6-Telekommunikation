const express = require("express");
const app = express();
const port = 8080;

// mock database
let usersDatabase = [{
        id: 0,
        pin: 2088290703, // 1234
        rfid: 3744025910 // card
    },
    //{
    //    id: 1,
    //    pin: 2088290703, // 1234
    //    rfid: 4239274179 // Blue chip
    //}
    {
        id: 1,
        pin: 0,
        rfid: 0
    },
    {
        id: 2,
        pin: 0,
        rfid: 0
    },
    {
        id: 3,
        pin: 0,
        rfid: 0
    },
    {
        id: 4,
        pin: 0,
        rfid: 0
    },

];

// middleware

// check if pin exists in the database and returns `authenticated: false` if not
const checkPin = (req, res, next) => {
    const pin = parseInt(req.params.pin);
    const user = usersDatabase.find(user => user.pin == pin);
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
    const rfid = parseInt(req.params.rfid);
    const user = usersDatabase.find(user => user.rfid == rfid);

    console.log(`rfid: ${rfid}, user: ${user}`)

    if (user) {
        console.log("success rfid found in database");
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
        id: user.id,
    });

});

app.get("/rfid/:rfid", checkRfid, (req, res) => {
    const user = req.user;

    res.json({
        authenticated: true,
        id: user.id,
    });
});

// Get database
app.get("/database", (req, res) => {
    res.json(usersDatabase);
});

// Update pin for user
app.put("/pin/:id/:pin", (req, res) => {
    const id = parseInt(req.params.id);
    const pin = parseInt(req.params.pin);

    // TODO: check if database goes to 5
    if(id < 5 && id >= 0 ) {
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
    const id = parseInt(req.params.id);
    const rfid = parseInt(req.params.rfid);

    console.log(`id: ${id}, rfid: ${rfid}`);

    if(id < 5 && id >= 0 ) {
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