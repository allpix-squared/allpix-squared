# DatabaseWriter
**Maintainer**: Enrico Junior Schioppa (<enrico.junior.schioppa@cern.ch>)  
**Status**: Functional  
**Input**: *all objects in simulation*

### Description
This module enables writing the simulation output into a postgreSQL database. This is useful when fast I/O between applications is needed (e.g. real time visualization and/or analysis).
By default, all object types (DepositedCharge, MCParticle, MCTrack, PixelCharge, PixelHit, PropagatedCharge) are written. However, bear in mind that PropagatedCharge and DepositedCharge data will slow down the simulation significantly. It is thus recommended to exclude these objects. This can be accomplished by using the `include` and `exclude` parameters in the configuration file.
In order to use this module, one is required to install postgreSQL and generate the database using the create-db.sql script in /etc/scripts. On Linux, this can be done as

```
$ sudo -u postgres psql
postgres=# create database mydb;
postgres=# \q
$ sudo -u postgres psql mydb
postgres=# \i create-db.sql
```

This generates a database with the following structure:

```
                            List of relations
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

To write into the database, a username and password are required. To create a new user/password pair and grant him/her privileges to edit the database, type


```
postgres=# create user myuser with encrypted password 'mypass';
postgres=# grant all privileges on database mydb to myuser;
```

### Parameters
* `host` : hostname of the machine holding the database, e.g. localhost
* `port` : port number (postgreSQL default is 5432)
* `dbname` : name of the database (mydb in the examples above)
* `user` : username (myuser in the examples above)
* `password` : password associated with the username (mypass in the examples above)
* `runID` : user-defined string identifying the simulation
* `include` : Array of object names (without `allpix::` prefix) to write to the database, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names (without `allpix::` prefix) that are not written to the database (cannot be used together simultaneously with the *include* parameter).

### Usage
Place the following configuration at the end of the main configuration file:

```ini
[DatabaseWriter]
```

The following parameters are mandatory: `host`, `port`, `dbname`, `user`, `password`. Again, it is recommended to exclude PropagatedCharge and DepositedCharge. A full example is thus

```ini
[DatabaseWriter]
exclude = "PropagatedCharge" "DepositedCharge"
host = "localhost"
port = "5432"
dbname = "mydb"
user = "myuser"
password = "mypass"
runID = "myRun"
```