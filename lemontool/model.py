import os 

import sqlalchemy as sa
import sqlalchemy.orm as orm

import config
DATABASENAME = config.DATABASENAME

#TODO the behaviour is completely wrong, I will want to be able to rerun lemon importer
# without completing destroying the database! That is it should allow incremental updates.
# This will not be hard to do, I just haven't done it . . .
if os.path.exists(DATABASENAME):
    os.remove(DATABASENAME)
engine = sa.create_engine('sqlite:///' + DATABASENAME)
metadata = sa.MetaData()
Session = orm.sessionmaker(bind=engine, autoflush=True, transactional=True)
session = Session()

# Define and create all tables
roms_table = sa.Table('games', metadata,
    sa.Column('id', sa.Integer, primary_key=True),
    sa.Column('filename', sa.String()),
    sa.Column('name', sa.String()),
    sa.Column('genre', sa.String()),
    sa.Column('clone_of', sa.String()),
    sa.Column('manufacturer', sa.String(), default='Unknown'),
    sa.Column('year', sa.Integer()),
    sa.Column('last_played', sa.DateTime()),
    sa.Column('params', sa.String()),
    sa.Column('count', sa.Integer()),
    sa.Column('favourite', sa.Boolean(), default=False),
    sa.Column('hide', sa.Boolean(), default=False),
    sa.Column('broken', sa.Boolean(), default=False),
    sa.Column('missing', sa.Boolean(), default=True),
    sa.Column('stars', sa.Integer()))

metadata.create_all(engine)

# Map to objects
class Rom(object):

    def __init__(self, filename):
        self.filename = filename
        self.count = 0
        self.stars = 0
        self.year = 0

    def __repr__(self):
        return "<Rom('%s', '%s', '%i')>" % (self.filename, self.genre,
                                            self.stars)

orm.mapper(Rom, roms_table)
