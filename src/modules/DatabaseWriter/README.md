# DatabaseWriter
**Maintainer**: Enrico Junior Schioppa (<enrico.junior.schioppa@cern.ch>)  
**Status**: Functional  
**Input**: *all objects in simulation*

### Description
This module enables writing the simulation output into a postgreSQL database. This is useful when fast I/O between applications is needed (e.g. real time visualization and/or analysis).
By default, all object types (MCTrack, MCParticle, DepositedCharge, PropagatedCharge, PixelCharge, PixelHit) are written. However, bear in mind that PropagatedCharge and DepositedCharge data will slow down the simulation significantly. It is thus recommended to exclude these objects. This can be accomplished by using the `include` and `exclude` parameters in the configuration file.
In order to use this module, one is required to install postgreSQL and generate the database using the create-db.sql script in /etc/scripts. On Linux, this can be done as

```
$ sudo -u postgres psql
postgres=# CREATE DATABASE mydb;
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
$ sudo -u postgres createuser myuser
$ sudo -u postgres psql mydb
postgres=# CREATE USER myuser WITH ENCRYPTED PASSWORD 'mypass';
postgres=# GRANT ALL PRIVILEGES ON DATABASE mydb TO myuser;
```

If an authentication failure error is issued, try

```
$ sudo -u postgres psql -c "ALTER USER myuser PASSWORD 'mypass';"
```

The database is structured so that the data are referenced according to the sequence

```
MCTrack -> MCParticle -> DepositedCharge -> PropagatedCharge -> PixelCharge -> PixelHit
```

This allows for the full reconstruction of the MC truth when retrieving information out of the database. When one of the objects is excluded, the corresponding reference is obviously lost and the chain is broken. The only exception to this chain rule is the direct reference MCParticle -> PixelHit. By default, each module always refers to the run and event numbers. As an example, the following is the table corresponding to the PixelHit objects for a single run:

```
mydb=# SELECT * FROM pixelhit;
 pixelhit_nr | run_nr | event_nr | mcparticle_nr | pixelcharge_nr | detector  | x | y | signal  | hittime 
-------------+--------+----------+---------------+----------------+-----------+---+---+---------+---------
           1 |      1 |        1 |             2 |              2 | detector1 | 2 | 2 | 46447.9 |       0
           2 |      1 |        1 |             2 |              2 | detector2 | 2 | 2 | 34847.5 |       0
           3 |      1 |        2 |             4 |              4 | detector1 | 2 | 2 | 27788.1 |       0
           4 |      1 |        2 |             4 |              4 | detector2 | 2 | 2 | 38011.6 |       0
           5 |      1 |        3 |             6 |              6 | detector1 | 2 | 2 | 30154.8 |       0
           6 |      1 |        3 |             6 |              6 | detector2 | 2 | 2 | 45947.4 |       0
           7 |      1 |        4 |             8 |              8 | detector1 | 2 | 2 | 51504.7 |       0
           8 |      1 |        4 |             8 |              8 | detector2 | 2 | 2 | 53500.9 |       0
           9 |      1 |        5 |            11 |             10 | detector1 | 2 | 2 | 29432.6 |       0
          10 |      1 |        5 |            11 |             10 | detector2 | 2 | 2 | 32942.5 |       0
          11 |      1 |        6 |            13 |             12 | detector1 | 2 | 2 | 50173.8 |       0
          12 |      1 |        6 |            13 |             12 | detector2 | 2 | 2 | 42945.5 |       0
          13 |      1 |        7 |            15 |             14 | detector1 | 2 | 2 | 33263.4 |       0
          14 |      1 |        7 |            15 |             14 | detector2 | 2 | 2 | 30430.5 |       0
          15 |      1 |        8 |            17 |             16 | detector1 | 2 | 2 | 33353.4 |       0
          16 |      1 |        8 |            17 |             16 | detector2 | 2 | 2 | 38631.7 |       0
          17 |      1 |        9 |            19 |             18 | detector1 | 2 | 2 | 30899.5 |       0
          18 |      1 |        9 |            19 |             18 | detector2 | 2 | 2 | 24996.9 |       0
          19 |      1 |       10 |            21 |             20 | detector1 | 2 | 2 | 29509.3 |       0
          20 |      1 |       10 |            21 |             20 | detector2 | 2 | 2 | 28590.8 |       0
```

### Parameters
See example in usage section, below.

### Usage
Place the following configuration at the end of the main configuration file:

```ini
[DatabaseWriter]
```

The following parameters are mandatory: `host`, `port`, `dbname`, `user`, `password`. Again, it is recommended to exclude PropagatedCharge and DepositedCharge using the `include` and `exclude` parameters. A full example is thus

```ini
[DatabaseWriter]
exclude = "PropagatedCharge" "DepositedCharge"
host = "localhost"
port = "5432"
database_name = "mydb"
user = "myuser"
password = "mypass"
run_id = "myRun"
```