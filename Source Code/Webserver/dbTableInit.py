import sqlite3

# SQLite DB Name
dbName =  "IoT.db" 

def createDB(s1Name, s2Name, deviceName):
    # SQLite DB Table Schema
    TableSchema=f"""
    drop table if exists {s1Name} ;
    create table {s1Name} (
    id integer primary key autoincrement,
    Status INTEGER,
    SN TEXT,
    Time TEXT,
    SoC INTEGER,
    SoH INTEGER
    );

    drop table if exists {s2Name} ;
    create table {s2Name} (
    id integer primary key autoincrement,
    Status INTEGER,
    SN TEXT,
    Time TEXT,
    SoC INTEGER,
    SoH INTEGER
    );

    drop table if exists {deviceName} ;
    create table {deviceName} (
    id integer primary key autoincrement,
    chargeMode INTEGER,
    MCC INTEGER,
    MNC INTEGER,
    LAC INTEGER,
    CellID INTEGER
    );
    """

    #Connect or Create DB File
    conn = sqlite3.connect(dbName)
    curs = conn.cursor()

    #Create Tables
    sqlite3.complete_statement(TableSchema)
    curs.executescript(TableSchema)

    #Close DB
    curs.close()
    conn.close()