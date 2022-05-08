import sqlite3
 
# Connecting to sqlite
# connection object
connection_obj = sqlite3.connect('IoT.db')
 
# cursor object
cursor_obj = connection_obj.cursor()
 
# Drop the GEEK table if already exists.
cursor_obj.execute("DROP TABLE IF EXISTS charger_status")
cursor_obj.execute("DROP TABLE IF EXISTS charger_enable")
 
# Creating table
table = """ CREATE TABLE charger_enable (
            id integer primary key autoincrement,
            productId INTEGER,
            productEnable INTEGER
        ); """
 
cursor_obj.execute(table)
 
print("Table is Ready")
 
# Close the connection
connection_obj.close()