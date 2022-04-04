# loading in modules
import sqlite3

def printTable(querySelect): 
    cur.execute(querySelect)
    col_name_list = [tuple[0] for tuple in cur.description]
    print (col_name_list)

    for row in cur.execute(querySelect):
        print(row)
    
    print("")

# creating file path
dbfile = 'user.db'
# Create a SQL connection to our SQLite database
con = sqlite3.connect(dbfile)

# creating cursor
cur = con.cursor()

# reading all table names
# table_list = [a for a in cur.execute("SELECT name FROM sqlite_master WHERE type = 'table'")]
# # here is you table list
# print(table_list)

# print("Slot 1")
printTable("SELECT * FROM " + "userinfo") 

# print("Slot 2")
# printTable("SELECT * FROM " + "charger3_s2") 

# print("Device")
# printTable("SELECT * FROM " + "charger3_device") 
 

# Be sure to close the connection
con.close()