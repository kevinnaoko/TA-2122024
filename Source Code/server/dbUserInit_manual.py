import sqlite3

# SQLite DB Name
dbName =  "user.db" 

def createDB(): 
    #Connect or Create DB File
    conn = sqlite3.connect(dbName)
    curs = conn.cursor()

    #Create Tables
    curs.execute('DROP TABLE IF EXISTS userinfo')
    curs.execute('''
    CREATE TABLE userinfo(
        id integer primary key autoincrement, 
        fullname TEXT,
        username TEXT, 
        password BLOB, 
        salt BLOB    
    )''')

    #Close DB
    curs.close()
    conn.close()
    
createDB()
 