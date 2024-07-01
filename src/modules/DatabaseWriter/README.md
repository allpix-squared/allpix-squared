---
# SPDX-FileCopyrightText: 2018-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DatabaseWriter"
description: "Writes simulation objects to a PostgreSQL database"
module_status: "Functional"
module_maintainers: ["Enrico Junior Schioppa (<enrico.junior.schioppa@cern.ch>)", "Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_inputs: ["all objects in simulation"]
---

## Description

This module enables writing the simulation output into a postgreSQL database.
This is useful when fast I/O between applications is needed (e.g. real time visualization and/or analysis).
By default, all object types (MCTrack, MCParticle, DepositedCharge, PropagatedCharge, PixelCharge, PixelHit) are written.
However, it should be kept in mind that PropagatedCharge and DepositedCharge data will slow down the simulation significantly and will lead to a large database.
Unless really required for the analysis of the simulation, it is recommended to exclude these objects.
This can be accomplished by using the `include` and `exclude` parameters in the configuration file.
In order to use this module, one is required to install PostgreSQL and generate a database using the `create-db.sql` script in `/etc/scripts`. On Linux, this can be done as

```shell
sudo -u postgres psql
postgres: CREATE DATABASE mydb;
postgres: \q
sudo -u postgres psql mydb
postgres: \i etc/scripts/create-db.sql
```

This generates a database with the following structure:

```shell
 Schema |                   Name                   |   Type   |  Owner
--------+------------------------------------------+----------+----------
 public | depositedcharge                          | table    | postgres
 public | depositedcharge_depositedcharge_nr_seq   | sequence | postgres
 public | event                                    | table    | postgres
 public | event_event_nr_seq                       | sequence | postgres
 public | mcparticle                               | table    | postgres
 public | mcparticle_mcparticle_nr_seq             | sequence | postgres
 public | mctrack                                  | table    | postgres
 public | mctrack_mctrack_nr_seq                   | sequence | postgres
 public | pixelcharge                              | table    | postgres
 public | pixelcharge_pixelcharge_nr_seq           | sequence | postgres
 public | pixelhit                                 | table    | postgres
 public | pixelhit_pixelhit_nr_seq                 | sequence | postgres
 public | propagatedcharge                         | table    | postgres
 public | propagatedcharge_propagatedcharge_nr_seq | sequence | postgres
 public | run                                      | table    | postgres
 public | run_run_nr_seq                           | sequence | postgres
(16 rows)
```

Host, username and password are required to write into the database.
A new user/password pair can be created and relevant privileges to edit the database can be created via

```shell
sudo -u postgres createuser myuser
sudo -u postgres psql mydb
postgres: CREATE USER myuser WITH ENCRYPTED PASSWORD 'mypass';
postgres: GRANT ALL PRIVILEGES ON DATABASE mydb TO myuser;
postgres: GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO myuser;
postgres: GRANT SELECT, USAGE ON ALL SEQUENCES IN SCHEMA public TO myuser;
```

In case of an authentication failure error being issues, the password of the user can be changed using

```shell
sudo -u postgres psql -c "ALTER USER myuser PASSWORD 'mypass';"
```

The database is structured so that the data are referenced according to the sequence

```shell
MCTrack -> MCParticle -> DepositedCharge -> PropagatedCharge -> PixelCharge -> PixelHit
```

This allows for the full reconstruction of the MC truth when retrieving information out of the database. When one of the objects is excluded, the corresponding reference is obviously lost and the chain is broken. The only exception to this chain rule is the direct reference MCParticle -> PixelHit. By default, each module always refers to the run and event numbers. As an example, the following is the table corresponding to the PixelHit objects for a single run of four events:

```shell
mydb: SELECT * FROM pixelhit;
 pixelhit_nr | run_nr | event_nr | mcparticle_nr | pixelcharge_nr | detector  | x | y | signal  | hittime
-------------+--------+----------+---------------+----------------+-----------+---+---+---------+---------
           1 |      1 |        1 |             2 |              2 | detector1 | 2 | 2 | 46447.9 |       0
           2 |      1 |        1 |             2 |              2 | detector2 | 2 | 2 | 34847.5 |       0
           3 |      1 |        2 |             4 |              4 | detector1 | 2 | 2 | 27788.1 |       0
           4 |      1 |        2 |             4 |              4 | detector2 | 2 | 2 | 38011.6 |       0
```

## Parameters

* `host`: Host address on which the database server runs, can be an IP address or host name. Mandatory parameter.
* `port`: Port the database server listens on. Mandatory parameter.
* `database_name`: Name of the database to store data in. The database needs to exist and has to be created before starting the simulation. Mandatory parameter.
* `user`: User name of the SQL user with access rights to the relevant database. mandatory parameter.
* `password`: Password of the user account with database write access. Mandatory parameter.
* `run_id`: Arbitrary run identifier assigned to this simulation in the database. This parameter is a string and defaults to `none`.
* `include`: Array of object names (without `allpix::` prefix) to write to the ROOT trees, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names (without `allpix::` prefix) that are not written to the ROOT trees (cannot be used together simultaneously with the *include* parameter).
* `global_timing`: Flag to select global timing information to be written to the database. By default, local information is written, i.e. only the local time information from the pixel hit in question. If enabled, the timestamp is set as the global time information of the object with respect to the event begin. Defaults to `false`.
* `require_sequence`: Boolean flag to select whether events have to be written in sequential order or can be stored in the order of processing. Defaults to `false`, writing events immediately. If strict adherence to the order of events is required, finished events are buffered until they can be written to the database. Since in this case database access happens single-threaded, this might impact the performance of the simulation.

## Usage

To write objects excluding `PropagatedCharge` and `DepositedCharge` to a PostgreSQL database running on `localhost` with user `myuser`, the following configuration can be placed at the end of the main configuration:

```ini
[DatabaseWriter]
exclude = PropagatedCharge, DepositedCharge
host = "localhost"
port = 5432
database_name = "mydb"
user = "myuser"
password = "mypass"
run_id = "myRun"
```

Optionally the password can also be provided via the command line only, using `allpix -c config.conf -o DatabaseWriter.password="mypass"`.
