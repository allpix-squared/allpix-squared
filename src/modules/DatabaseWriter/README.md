TEST
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

The database is structured so that the data are referenced according to the sequence

```
Run -> Event -> MCTrack -> MCParticle -> DepositedCharge -> PropagatedCharge -> PixelCharge -> PixelHit
```

This allows for the full reconstruction of the MC truth when retrieving information out of the database. When one of the objects is excluded, the corresponding reference is obviously lost. However, the chain is not broken, as variables from parent objects are repeated down the chain. As an example, the following is the table corresponding to the PixelHit objects for two runs. In the first run, the MCTrack, DepositedCharge and PropagatedCharge object were excluded:

```
 pixelhit_nr | run_nr | event_nr | mctrack_nr | mcparticle_nr | depositedcharge_nr | propagatedcharge_nr | pixelcharge_nr | detector  | x | y | signal  | hittime 
-------------+--------+----------+------------+---------------+--------------------+---------------------+----------------+-----------+---+---+---------+---------
           1 |     15 |       17 |            |             8 |                    |                     |              2 | detector1 | 2 | 2 |   33086 |       0
           2 |     15 |       17 |            |             8 |                    |                     |              2 | detector2 | 2 | 2 | 41169.8 |       0
           3 |     15 |       18 |            |            10 |                    |                     |              4 | detector1 | 2 | 2 | 32975.9 |       0
           4 |     15 |       18 |            |            10 |                    |                     |              4 | detector2 | 2 | 2 | 51000.1 |       0
           5 |     15 |       19 |            |            12 |                    |                     |              6 | detector1 | 2 | 2 | 27643.9 |       0
           6 |     15 |       19 |            |            12 |                    |                     |              6 | detector2 | 2 | 2 | 36635.8 |       0
           7 |     15 |       20 |            |            15 |                    |                     |              8 | detector1 | 2 | 2 | 84205.4 |       0
           8 |     15 |       20 |            |            15 |                    |                     |              8 | detector2 | 2 | 2 |   29397 |       0
           9 |     15 |       21 |            |            17 |                    |                     |             10 | detector1 | 2 | 2 | 31162.2 |       0
          10 |     15 |       21 |            |            17 |                    |                     |             10 | detector2 | 2 | 2 | 40265.2 |       0
          11 |     15 |       22 |            |            19 |                    |                     |             12 | detector1 | 2 | 2 | 35163.8 |       0
          12 |     15 |       22 |            |            19 |                    |                     |             12 | detector2 | 2 | 2 | 35073.5 |       0
          13 |     15 |       23 |            |            21 |                    |                     |             14 | detector1 | 2 | 2 | 46394.8 |       0
          14 |     15 |       23 |            |            21 |                    |                     |             14 | detector2 | 2 | 2 | 27625.7 |       0
          15 |     15 |       24 |            |            23 |                    |                     |             16 | detector1 | 2 | 2 | 41677.1 |       0
          16 |     15 |       24 |            |            23 |                    |                     |             16 | detector2 | 2 | 2 | 25725.8 |       0
          17 |     15 |       25 |            |            25 |                    |                     |             18 | detector1 | 2 | 2 |   38990 |       0
          18 |     15 |       25 |            |            25 |                    |                     |             18 | detector2 | 2 | 2 | 36419.7 |       0
          19 |     15 |       26 |            |            27 |                    |                     |             20 | detector1 | 2 | 2 | 45989.8 |       0
          20 |     15 |       26 |            |            27 |                    |                     |             20 | detector2 | 2 | 2 | 26912.7 |       0
          21 |     17 |       28 |          3 |            33 |               4266 |                2254 |             24 | detector1 | 2 | 2 | 32616.2 |       0
          22 |     17 |       28 |          3 |            33 |               4266 |                2254 |             24 | detector2 | 0 | 2 | 6808.33 |       0
          23 |     17 |       28 |          3 |            33 |               4266 |                2254 |             24 | detector2 | 1 | 2 | 25265.5 |       0
          24 |     17 |       28 |          3 |            33 |               4266 |                2254 |             24 | detector2 | 2 | 2 | 78604.9 |       0
          25 |     17 |       29 |          4 |            35 |               5784 |                3324 |             26 | detector1 | 2 | 2 | 26531.5 |       0
          26 |     17 |       29 |          4 |            35 |               5784 |                3324 |             26 | detector2 | 2 | 2 | 37189.6 |       0
          27 |     17 |       30 |          6 |            38 |               7508 |                4753 |             28 | detector1 | 2 | 2 | 32983.8 |       0
          28 |     17 |       30 |          6 |            38 |               7508 |                4753 |             28 | detector2 | 2 | 2 |   60854 |       0
          29 |     17 |       31 |          8 |            41 |               9276 |                6260 |             30 | detector1 | 2 | 2 | 34785.8 |       0
          30 |     17 |       31 |          8 |            41 |               9276 |                6260 |             30 | detector2 | 2 | 2 | 64764.6 |       0
          31 |     17 |       32 |          9 |            43 |              10822 |                7450 |             32 | detector1 | 2 | 2 | 32901.3 |       0
          32 |     17 |       32 |          9 |            43 |              10822 |                7450 |             32 | detector2 | 2 | 2 | 41381.4 |       0
          33 |     17 |       33 |         10 |            45 |              12364 |                8563 |             34 | detector1 | 2 | 2 | 34940.1 |       0
          34 |     17 |       33 |         10 |            45 |              12364 |                8563 |             34 | detector2 | 2 | 2 | 30685.5 |       0

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

The following parameters are mandatory: `host`, `port`, `dbname`, `user`, `password`. Again, it is recommended to exclude PropagatedCharge and DepositedCharge using the `include` and `exclude` parameters. A full example is thus

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