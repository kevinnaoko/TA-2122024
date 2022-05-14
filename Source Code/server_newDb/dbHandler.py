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
def slotHandler(jsonData, suffix):
    try:
        jsonRows = json.loads(jsonData)
        
        productID = jsonRows['Product ID']
        chgStatus = jsonRows['Status']
        battSN = jsonRows['Serial Number']
        time = jsonRows['Time']
        battSoC = jsonRows['SoC']
        battSoH = jsonRows['SoH']
        battVolt = jsonRows['Volt']
        chgMode = jsonRows['Charging Mode']

        # Sisipkan data ke db
        dbObj = dbManager() 
        tableName = "charger_" + suffix
        sqliteQuery = "INSERT INTO " + tableName + " (productId, Status, SN, Time, SoC, SoH, Volt, chargeMode) values (?,?,?,?,?,?,?,?)" 
        dbObj.insertRecord(sqliteQuery, [productID, chgStatus, battSN, time, battSoC, battSoH, battVolt, chgMode])
        del dbObj

        print ("Inserted " + suffix + " into " + tableName)
        print ("")
    except:
        print ('insertion failed, invalid json format')

def deviceHandler(jsonData):
    try:
        jsonRows = json.loads(jsonData)
        
        productID = jsonRows['Product ID']
        chgMode = jsonRows['Charging Mode']
        locMCC = jsonRows['MCC']
        locMNC = jsonRows['MNC']
        locLAC = jsonRows['LAC']
        locCellID = jsonRows['Cell ID']

        # Sisipkan data ke db
        dbObj = dbManager() 
        tableName = "charger_device"
        sqliteQuery = "INSERT INTO " + tableName + " (productId, chargeMode, MCC, MNC, LAC, CellID) values (?,?,?,?,?,?)" 
        dbObj.insertRecord(sqliteQuery, [productID, chgMode, locMCC, locMNC, locLAC, locCellID])
        del dbObj

        print ("Inserted device info into " + tableName)
        print ("")

    except:
        print ('insertion failed, invalid json format')

def dataHandler(Topic, jsonData):
    #split topic (sys/s1)
    #concatTopic[0] berisi sys
    #concatTopic[2] berisi parameter dabel(s1/s2/device)
    concatTopic = Topic.split("/")

    if concatTopic[1] == "s1" or concatTopic[1] == "s2":
        slotHandler(jsonData, concatTopic[1]) 
    elif concatTopic[1] == "device": 
        deviceHandler(jsonData)	
    