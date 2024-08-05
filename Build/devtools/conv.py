import sqlite3

conn = sqlite3.connect('mydatabase.sqlite')
conn.execute('DROP TABLE IF EXISTS spatial_ref_sys')
conn.execute('CREATE TABLE spatial_ref_sys (srid INTEGER PRIMARY KEY, auth_name TEXT, auth_srid INTEGER, mid TEXT, srtext TEXT)')
conn.commit()
conn.close()

import os
import pyproj
import sqlite3

conn = sqlite3.connect('mydatabase.sqlite')
crs = pyproj.CRS.from_user_input('epsg:4326')
cursor = conn.cursor()
d = 0
for file in os.listdir('D:/Programs/imgui/GeoBar/Release/EPSG'):
    if file.endswith('.wkt'):
        with open(os.path.join('D:/Programs/imgui/GeoBar/Release/EPSG', file), 'r') as f:
            wkt = f.read().strip()
            fullname = file.split('.')[0]
            parts = fullname.split('-')
            srid = int(parts[-1])
            auth_name = 'EPSG'
            auth_srid = srid

            a = cursor.execute('INSERT INTO spatial_ref_sys (srid, auth_name, auth_srid, mid, srtext) VALUES (?, ?, ?, ?, ?)',
                         (srid, auth_name, auth_srid, parts[1], wkt))


conn.commit()
conn.close()
