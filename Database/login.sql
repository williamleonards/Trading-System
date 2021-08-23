-- Table: ts.login

-- DROP TABLE ts.login;

CREATE TABLE IF NOT EXISTS ts.login
(
    username character varying(20) COLLATE pg_catalog."default" NOT NULL,
    password character varying(20) COLLATE pg_catalog."default" NOT NULL,
    CONSTRAINT login_pkey PRIMARY KEY (username)
)

TABLESPACE pg_default;

ALTER TABLE ts.login
    OWNER to postgres;
-- Index: idx_login_username

-- DROP INDEX ts.idx_login_username;

CREATE INDEX idx_login_username
    ON ts.login USING hash
    (username COLLATE pg_catalog."default")
    TABLESPACE pg_default;