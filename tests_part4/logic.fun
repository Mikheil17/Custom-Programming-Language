PROGRAM LOGICFUN;
VAR
    X : INTEGER;
    Y : INTEGER;
    Z : INTEGER;
    SCORE : INTEGER;
BEGIN
    SCORE := 0;

    WRITE('Enter X.');
    READ(X);

    WRITE('Enter Y.');
    READ(Y);

    WRITE('Enter Z.');
    READ(Z);

    IF ((2 + X) OR Y AND 3 < 4) THEN
    BEGIN
        WRITE('Test 1 passed.');
        SCORE := SCORE + 1
    END
    ELSE
        WRITE('Test 1 failed.');

    IF ((X * 2 < Y + 3) AND (Z - 1)) THEN
    BEGIN
        WRITE('Test 2 passed.');
        SCORE := SCORE + 1
    END
    ELSE
        WRITE('Test 2 failed.');

    IF ((X + Y * Z) AND (X < Y OR Z > 0)) THEN
    BEGIN
        WRITE('Test 3 passed.');
        SCORE := SCORE + 1
    END
    ELSE
        WRITE('Test 3 failed.');

    IF ((X - X) OR (Y - 2) OR (Z < 10)) THEN
    BEGIN
        WRITE('Test 4 passed.');
        SCORE := SCORE + 1
    END
    ELSE
        WRITE('Test 4 failed.');

    WRITE('Final weird logic score:');
    WRITE(SCORE);

    IF (SCORE > 3) THEN
        WRITE('You survived the haunted truth table.')
    ELSE
        WRITE('The booleans have rejected your offering.')
END