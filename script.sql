CREATE TABLE users (id int, name str);
INSERT INTO users (id, name) VALUES (1, "Alice"), (2, "Bob"), (3, "Cara");
SELECT * FROM users;
SELECT name FROM users WHERE id != 2;
