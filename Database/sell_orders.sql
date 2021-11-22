-- Table: ts.sell_orders

-- DROP TABLE ts.sell_orders;

CREATE TABLE IF NOT EXISTS ts.sell_orders
(
    order_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
    username character varying(20) COLLATE pg_catalog."default" NOT NULL,
    amount integer NOT NULL,
    price integer NOT NULL,
    ticker character varying(5) COLLATE pg_catalog."default" NOT NULL,
    datetime bigint NOT NULL,
    CONSTRAINT pk_sell_orders PRIMARY KEY (order_id),
    CONSTRAINT fk_sell_orders_login FOREIGN KEY (username)
        REFERENCES ts.login (username) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION
        NOT VALID
)

TABLESPACE pg_default;

ALTER TABLE ts.sell_orders
    OWNER to postgres;
-- Index: idx_sell_orders_price

-- DROP INDEX ts.idx_sell_orders_price;

CREATE INDEX idx_sell_orders_price
    ON ts.sell_orders USING btree
    (price ASC NULLS LAST)
    TABLESPACE pg_default;

ALTER TABLE ts.sell_orders
    CLUSTER ON idx_sell_orders_price;