-- Table: ts.buy_orders

-- DROP TABLE ts.buy_orders;

CREATE TABLE IF NOT EXISTS ts.buy_orders
(
    order_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
    username character varying(20) COLLATE pg_catalog."default" NOT NULL,
    amount integer NOT NULL,
    price integer NOT NULL,
    valid_bit boolean NOT NULL,
    CONSTRAINT pk_buy_orders PRIMARY KEY (order_id),
    CONSTRAINT fk_buy_orders_login FOREIGN KEY (username)
        REFERENCES ts.login (username) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION
        NOT VALID
)

TABLESPACE pg_default;

ALTER TABLE ts.buy_orders
    OWNER to postgres;
-- Index: idx_buy_orders_price

-- DROP INDEX ts.idx_buy_orders_price;

CREATE INDEX idx_buy_orders_price
    ON ts.buy_orders USING btree
    (price DESC NULLS LAST)
    TABLESPACE pg_default;

ALTER TABLE ts.buy_orders
    CLUSTER ON idx_buy_orders_price;