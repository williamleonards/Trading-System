-- SCHEMA: ts

-- DROP SCHEMA ts ;

CREATE SCHEMA ts
    AUTHORIZATION postgres;

COMMENT ON SCHEMA ts
    IS 'standard public schema';

GRANT ALL ON SCHEMA ts TO PUBLIC;

GRANT ALL ON SCHEMA ts TO postgres;