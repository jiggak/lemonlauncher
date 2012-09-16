CREATE TABLE games (
   filename     TEXT PRIMARY KEY,
   name         TEXT NOT NULL,
   genre        TEXT NOT NULL DEFAULT 'Unknown',
   clone_of     TEXT DEFAULT NULL,
   manufacturer TEXT NOT NULL DEFAULT 'Unknown',
   year         INTEGER NOT NULL DEFAULT 0,
   last_played  TIMESTAMP,
   params       TEXT,
   count        INTEGER NOT NULL DEFAULT 0,
   favourite    BOOLEAN NOT NULL DEFAULT FALSE,
   hide         BOOLEAN NOT NULL DEFAULT FALSE,
   broken       BOOLEAN NOT NULL DEFAULT FALSE,
   missing      BOOLEAN NOT NULL DEFAULT TRUE
);
