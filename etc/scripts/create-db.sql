-- SPDX-FileCopyrightText: 2020-2025 CERN and the Allpix Squared authors
-- SPDX-License-Identifier: MIT

DROP TABLE IF EXISTS MCTrack;
DROP TABLE IF EXISTS MCParticle;
DROP TABLE IF EXISTS Propagatedcharge;
DROP TABLE IF EXISTS Depositedcharge;
DROP TABLE IF EXISTS PixelCharge;
DROP TABLE IF EXISTS PixelHit;
DROP TABLE IF EXISTS Event;
DROP TABLE IF EXISTS Run;

CREATE TABLE Run(
    run_nr SERIAL PRIMARY KEY,
    run_id VARCHAR (100)
);

CREATE TABLE Event(
    event_nr SERIAL,
    run_nr INT REFERENCES Run (run_nr),
    eventID INT NOT NULL,
    PRIMARY KEY (event_nr)
);

CREATE TABLE MCTrack(
    mctrack_nr SERIAL,
    run_nr INT REFERENCES Run (run_nr),
    event_nr INT REFERENCES Event (event_nr),
    detector VARCHAR (100) NOT NULL,
    address BIGINT NOT NULL,
    parentAddress BIGINT NOT NULL,
    particleID INT NOT NULL,
    productionProcess VARCHAR (100) NOT NULL,
    productionVolume VARCHAR (100) NOT NULL,
    initialPositionX FLOAT NOT NULL,
    initialPositionY FLOAT NOT NULL,
    initialPositionZ FLOAT NOT NULL,
    finalPositionX FLOAT NOT NULL,
    finalPositionY FLOAT NOT NULL,
    finalPositionZ FLOAT NOT NULL,
    initialTime FLOAT,
    finalTime FLOAT,
    initialKineticEnergy FLOAT NOT NULL,
    finalKineticEnergy FLOAT NOT NULL,
    PRIMARY KEY (mctrack_nr)
);

CREATE TABLE MCParticle(
    mcparticle_nr SERIAL,
    run_nr INT REFERENCES Run (run_nr),
    event_nr INT REFERENCES Event (event_nr),
    mctrack_nr INT REFERENCES MCtrack (mctrack_nr),
    detector VARCHAR (100) NOT NULL,
    address BIGINT NOT NULL,
    parentAddress BIGINT NOT NULL,
    trackAddress BIGINT NOT NULL,
    particleID INT NOT NULL,
    localStartPointX FLOAT NOT NULL,
    localStartPointY FLOAT NOT NULL,
    localStartPointZ FLOAT NOT NULL,
    localEndPointX FLOAT NOT NULL,
    localEndPointY FLOAT NOT NULL,
    localEndPointZ FLOAT NOT NULL,
    globalStartPointX FLOAT NOT NULL,
    globalStartPointY FLOAT NOT NULL,
    globalStartPointZ FLOAT NOT NULL,
    globalEndPointX FLOAT NOT NULL,
    globalEndPointY FLOAT NOT NULL,
    globalEndPointZ FLOAT NOT NULL,
    PRIMARY KEY (mcparticle_nr)
);

CREATE TABLE Depositedcharge(
    depositedcharge_nr SERIAL,
    run_nr INT REFERENCES Run (run_nr),
    event_nr INT REFERENCES Event (event_nr),
    mcparticle_nr INT REFERENCES MCParticle (mcparticle_nr),
    detector VARCHAR (100) NOT NULL,
    carriertype INT NOT NULL,
    charge INT NOT NULL,
    localx FLOAT NOT NULL,
    localy FLOAT NOT NULL,
    localz FLOAT NOT NULL,
    globalx FLOAT NOT NULL,
    globaly FLOAT NOT NULL,
    globalz FLOAT NOT NULL,
    PRIMARY KEY (depositedcharge_nr)
);

CREATE TABLE Propagatedcharge(
    propagatedcharge_nr SERIAL,
    run_nr INT REFERENCES Run (run_nr),
    event_nr INT REFERENCES Event (event_nr),
    depositedcharge_nr INT REFERENCES Depositedcharge (depositedcharge_nr),
    detector VARCHAR (100) NOT NULL,
    carriertype INT NOT NULL,
    charge INT NOT NULL,
    localx FLOAT NOT NULL,
    localy FLOAT NOT NULL,
    localz FLOAT NOT NULL,
    globalx FLOAT NOT NULL,
    globaly FLOAT NOT NULL,
    globalz FLOAT NOT NULL,
    PRIMARY KEY (propagatedcharge_nr)
);

CREATE TABLE PixelCharge(
    pixelCharge_nr SERIAL,
    run_nr INT REFERENCES Run (run_nr),
    event_nr INT REFERENCES Event (event_nr),
    propagatedcharge_nr INT REFERENCES Propagatedcharge (propagatedcharge_nr),
    detector VARCHAR (100) NOT NULL,
    charge INT NOT NULL,
    x INT NOT NULL,
    y INT NOT NULL,
    localx FLOAT NOT NULL,
    localy FLOAT NOT NULL,
    globalx FLOAT NOT NULL,
    globaly FLOAT NOT NULL,
    PRIMARY KEY (pixelCharge_nr)
);

CREATE TABLE PixelHit(
    pixelHit_nr SERIAL,
    run_nr INT REFERENCES Run (run_nr),
    event_nr INT REFERENCES Event (event_nr),
    mcparticle_nr INT REFERENCES MCParticle (mcparticle_nr),
    pixelcharge_nr INT REFERENCES Pixelcharge (pixelcharge_nr),
    detector VARCHAR (100) NOT NULL,
    x INT NOT NULL,
    y INT NOT NULL,
    signal FLOAT NOT NULL,
    hitTime INT NOT NULL,
    PRIMARY KEY (pixelHit_nr)
);

GRANT ALL PRIVILEGES ON TABLE Run TO myuser;
GRANT ALL PRIVILEGES ON TABLE Event TO myuser;
GRANT ALL PRIVILEGES ON TABLE PixelHit TO myuser;
GRANT ALL PRIVILEGES ON TABLE PixelCharge TO myuser;
GRANT ALL PRIVILEGES ON TABLE PropagatedCharge TO myuser;
GRANT ALL PRIVILEGES ON TABLE DepositedCharge TO myuser;
GRANT ALL PRIVILEGES ON TABLE MCTrack TO myuser;
GRANT ALL PRIVILEGES ON TABLE MCParticle TO myuser;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public to myuser;
