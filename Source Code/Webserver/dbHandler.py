import json
import sqlite3 

# SQLite DB Name
dbName = "IoT.db"
# Database Manager Class 
class dbManager():
    # load db, set key, set cursor
	def __init__(self):
		self.conn = sqlite3.connect(dbName)
		self.conn.execute('PRAGMA foreign_keys = on')
		self.conn.commit()
		self.cur = self.conn.cursor()
	
    # commit info ke newline di db
	def insertRecord(self, query, values=()):
		self.cur.execute(query, values)
		self.conn.commit()
		return

    # close cursor, close db
	def __del__(self):
		self.cur.close()
		self.conn.close()

# Functions
def slotHandler(jsonData, chargerPrefix, slotSuffix):
    jsonRows = json.loads(jsonData)
    chgStatus = jsonRows['Status']
    battSN = jsonRows['Serial Number']
    time = jsonRows['Time']
    battSoC = jsonRows['SoC']

    # Sisipkan data ke db
    dbObj = dbManager() 
    tableName = chargerPrefix + "_" + slotSuffix
    sqliteQuery = "INSERT INTO " + tableName + " (Status, SN, Time, SoC) values (?,?,?,?)" 
    dbObj.insertRecord(sqliteQuery, [chgStatus, battSN, time, battSoC])
    del dbObj

    print ("Inserted " + slotSuffix + " into " + tableName)
    print ("")

def deviceHandler(jsonData, chargerPrefix):
    jsonRows = json.loads(jsonData)
    chgMode = jsonRows['Charging Mode']
    locMCC = jsonRows['MCC']
    locMNC = jsonRows['MNC']
    locLAC = jsonRows['LAC']
    locCellID = jsonRows['Cell ID']

    # Sisipkan data ke db
    dbObj = dbManager() 
    tableName = chargerPrefix + "_device"
    sqliteQuery = "INSERT INTO " + tableName + " (chargeMode, MCC, MNC, LAC, CellID) values (?,?,?,?,?)" 
    dbObj.insertRecord(sqliteQuery, [chgMode, locMCC, locMNC, locLAC, locCellID])
    del dbObj

    print ("Inserted device info into " + tableName)
    print ("")

    test = 1

def dataHandler(Topic, jsonData):
    #split topic
    #concatTopic[1] berisi ID charger (charger1/charger2/...)
    #concatTopic[2] berisi parameter charger (s1/s2/device)
    concatTopic = Topic.split("/")

    if concatTopic[2] == "s1" or concatTopic[2] == "s2":
        slotHandler(jsonData, concatTopic[1], concatTopic[2]) 
    elif concatTopic[2] == "device": 
        deviceHandler(jsonData, concatTopic[1])	
    