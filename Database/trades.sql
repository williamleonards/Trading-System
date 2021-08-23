-- Table: ts.trades

-- DROP TABLE ts.trades;

CREATE TABLE IF NOT EXISTS ts.trades
(
    trade_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
    amount integer NOT NULL,
    price integer NOT NULL,
    buyer character varying(20) COLLATE pg_catalog."default" NOT NULL,
    seller character varying(20) COLLATE pg_catalog."default" NOT NULL,
    CONSTRAINT pk_trades PRIMARY KEY (trade_id),
    CONSTRAINT fk_trades_login_buyer FOREIGN KEY (buyer)
        REFERENCES ts.login (username) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION,
    CONSTRAINT fk_trades_login_seller FOREIGN KEY (seller)
        REFERENCES ts.login (username) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION
)

TABLESPACE pg_default;

ALTER TABLE ts.trades
    OWNER to postgres;
-- Index: idx_trades_buyer

-- DROP INDEX ts.idx_trades_buyer;

CREATE INDEX idx_trades_buyer
    ON ts.trades USING hash
    (buyer COLLATE pg_catalog."default")
    TABLESPACE pg_default;
-- Index: idx_trades_seller

-- DROP INDEX ts.idx_trades_seller;

CREATE INDEX idx_trades_seller
    ON ts.trades USING hash
    (seller COLLATE pg_catalog."default")
    TABLESPACE pg_default;