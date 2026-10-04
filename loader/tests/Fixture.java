import java.util.ArrayList;
import java.util.HashMap;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.zip.ZipFile;
import java.util.zip.ZipEntry;
import java.io.InputStream;
import java.util.function.IntUnaryOperator;

public final class Fixture {
    private int value;
    private static int initializedValue = 11;
    public static String constantString = "constant";
    private Fixture() {}
    private Fixture(int value) {
        this.value = value;
    }
    public static int answer() {
        return 42;
    }
    public static int staticValue() {
        return initializedValue;
    }
    public static int constantValue() {
        return 123456;
    }
    public static int crossClass() {
        return Other.addFive(7);
    }
    public static int nativeMath() {
        return Math.abs(-9);
    }
    public static int nativeArrayCopy() {
        int[] source = {1, 2};
        int[] target = new int[2];
        System.arraycopy(source, 0, target, 0, 2);
        return target[1];
    }
    public static int nativeClock() {
        return System.currentTimeMillis() >= 0L ? 1 : 0;
    }
    public static int platformValue() {
        return System.getProperty("java.version").length() + Runtime.getRuntime().availableProcessors();
    }
    public static int threadValue() {
        Thread thread = Thread.currentThread();
        return thread.getName().length() + thread.getPriority()
            + (thread.isAlive() ? 1 : 0) + (Thread.activeCount() == 1 ? 1 : 0);
    }
    public static int coreNativeValue() throws InterruptedException {
        Object monitor = new Object();
        monitor.notify();
        monitor.notifyAll();
        monitor.wait(0L);
        return String.class.desiredAssertionStatus() ? 0 : 1;
    }
    public static int numericNativeValue() {
        float value = Float.intBitsToFloat(Float.floatToRawIntBits(3.5f));
        return (int) value + (int) Math.floor(2.9) + (Math.cos(0.0) == 1.0 ? 1 : 0);
    }
    public static int propertyValue() {
        String home = System.getProperty("java.home", "missing");
        String encoding = System.getProperty("file.encoding", "missing");
        return (home.length() > 0 ? 1 : 0) + (encoding.equals("UTF-8") ? 2 : 0)
            + (System.getenv("JVMR_TEST_MISSING") == null ? 4 : 0);
    }
    public static int platformLibraryValue() {
        return System.mapLibraryName("jvmr").endsWith(".so") ? 1 : 0;
    }
    public static int fileValue() {
        File current = new File(".");
        File fixture = new File("loader/build/Fixture.class");
        return (current.exists() ? 1 : 0) + (current.isDirectory() ? 2 : 0)
            + (fixture.isFile() ? 4 : 0) + (fixture.length() > 0 ? 8 : 0);
    }
    public static int streamValue() throws Exception {
        FileInputStream stream = new FileInputStream("loader/build/Fixture.class");
        int first = stream.read();
        byte[] next = new byte[3];
        int count = stream.read(next, 0, 3);
        stream.close();
        return (first == 0xca && count == 3 && (next[0] & 0xff) == 0xfe
            && (next[1] & 0xff) == 0xba && (next[2] & 0xff) == 0xbe) ? 1 : 0;
    }
    public static int outputStreamValue() throws Exception {
        FileOutputStream output = new FileOutputStream("loader/build/jvmr-output.bin");
        output.write(new byte[] { 7, 8, 9 });
        output.close();
        FileInputStream input = new FileInputStream("loader/build/jvmr-output.bin");
        byte[] bytes = new byte[3];
        int count = input.read(bytes);
        input.close();
        return count == 3 && bytes[0] == 7 && bytes[1] == 8 && bytes[2] == 9 ? 1 : 0;
    }
    public static int nioValue() throws Exception {
        Path path = Path.of("loader/build/Fixture.class");
        byte[] bytes = Files.readAllBytes(path);
        return (Files.exists(path) ? 1 : 0) + (bytes[0] == (byte) 0xca ? 2 : 0)
            + (Files.size(path) > 0 ? 4 : 0);
    }
    public static int nioExtendedValue() throws Exception {
        Path classFile = Path.of("loader/build/Fixture.class");
        Path sourceDirectory = Path.of("loader/tests");
        String source = Files.readString(Path.of("loader/tests/Fixture.java"));
        return (Files.isRegularFile(classFile) ? 1 : 0)
            + (Files.isDirectory(sourceDirectory) ? 2 : 0)
            + (source.contains("class Fixture") ? 4 : 0);
    }
    public static int nioWriteValue() throws Exception {
        Path directory = Path.of("loader/build/jvmr-nio");
        Path file = Path.of("loader/build/jvmr-nio/data.txt");
        Files.createDirectories(directory);
        Files.writeString(file, "jvmr");
        return Files.readString(file).equals("jvmr") ? 1 : 0;
    }
    public static int zipValue() throws Exception {
        ZipFile zip = new ZipFile("loader/build/fixture.jar");
        ZipEntry entry = zip.getEntry("Fixture.class");
        InputStream input = zip.getInputStream(entry);
        int a = input.read(), b = input.read(), c = input.read(), d = input.read();
        input.close();
        zip.close();
        return entry != null && a == 0xca && b == 0xfe && c == 0xba && d == 0xbe ? 1 : 0;
    }
    public static int resourceValue() throws Exception {
        InputStream input = Fixture.class.getResourceAsStream("/Fixture.class");
        int a = input.read(), b = input.read(), c = input.read(), d = input.read();
        input.close();
        return a == 0xca && b == 0xfe && c == 0xba && d == 0xbe ? 1 : 0;
    }
    public static int stringExtendedValue() {
        String value = "  fabric-loader-1.21  ".trim().replace("loader", "runtime").concat("!");
        return value.equals("fabric-runtime-1.21!") && value.lastIndexOf("21") == 17
            && value.getBytes().length == value.toCharArray().length ? 1 : 0;
    }
    public static int classLoaderValue() throws Exception {
        ClassLoader loader = Thread.currentThread().getContextClassLoader();
        return loader != null && loader.loadClass("Fixture") == Fixture.class ? 1 : 0;
    }
    public static int argLength(String[] args) {
        return args.length;
    }
    public static void main(String[] args) {
        argLength(args);
    }
    public static int identityValue() {
        Object value = new Object();
        return System.identityHashCode(value) != 0 ? 1 : 0;
    }
    public static int classValue() {
        Class<String> stringType = String.class;
        return stringType.getName().length()
            + (Object.class.isAssignableFrom(String.class) ? 1 : 0)
            + (stringType.isInstance("x") ? 1 : 0);
    }
    public static int objectClassValue() {
        return new Object().getClass().getName().length()
            + (String.class.getSuperclass() == Object.class ? 1 : 0)
            + (String.class.isInterface() ? 1 : 0);
    }
    public static int forNameValue() throws ClassNotFoundException {
        return Class.forName("java.lang.String") == String.class ? 1 : 0;
    }
    public static int memberReflectionValue() {
        boolean foundMethod = false;
        boolean foundField = false;
        for (Method method : String.class.getDeclaredMethods())
            if (method.getName().equals("length") && method.getParameterCount() == 0) foundMethod = true;
        for (Field field : Integer.class.getDeclaredFields())
            if (field.getName().equals("value")) foundField = true;
        return (foundMethod ? 2 : 0) | (foundField ? 1 : 0);
    }
    public static int methodLookupValue() throws Exception {
        return String.class.getDeclaredMethod("length").getParameterCount();
    }
    public static int fieldReflectionValue() throws Exception {
        Field number = Fixture.class.getDeclaredField("initializedValue");
        return number.getInt(null);
    }
    public static int reflectedInvokeValue() throws Exception {
        Method method = Fixture.class.getDeclaredMethod("answer");
        return ((Integer) method.invoke(null)).intValue();
    }
    public static int reflectedInstanceInvokeValue() throws Exception {
        Method method = Fixture.class.getDeclaredMethod("getValue");
        return ((Integer) method.invoke(new Fixture(37))).intValue();
    }
    public static int catchesArithmetic() {
        try {
            return 1 / 0;
        } catch (ArithmeticException exception) {
            return 7;
        }
    }
    public static int catchesRuntime() {
        try {
            return 1 / 0;
        } catch (RuntimeException exception) {
            return 8;
        }
    }
    public static int stringValue() {
        String value = "hello";
        return value.length() + value.charAt(1);
    }
    public static int stringEquals() {
        return "same".equals("same") ? 1 : 0;
    }
    public static int stringOpsValue() {
        String value = "Minecraft/Fabric";
        return value.substring(0, 9).length() + value.indexOf("Fabric")
            + (value.startsWith("Mine") ? 1 : 0)
            + (value.endsWith("ric") ? 1 : 0)
            + (value.toLowerCase().contains("craft") ? 1 : 0);
    }
    public static int builderLength() {
        return new StringBuilder().append("abc").append(123).toString().length();
    }
    public static int builderRangeValue() {
        return new StringBuilder().append("abcdef", 1, 4).toString().equals("bcd") ? 1 : 0;
    }
    public static String concatValue() {
        return "value=" + 7;
    }
    public static int interfaceValue() {
        Adder adder = new Impl();
        return adder.add(6);
    }
    public static int inheritedValue() {
        return new Derived().baseValue() * 2;
    }
    public static int inheritedFieldValue() {
        return new Derived().inheritedField();
    }
    public static int constantStringLength() {
        return constantString.length();
    }
    public static int listValue() {
        ArrayList<Integer> values = new ArrayList<>();
        values.add(4);
        values.add(8);
        return values.size() + values.get(1);
    }
    public static int mapValue() {
        HashMap<String, Integer> values = new HashMap<>();
        values.put("answer", 39);
        return values.get("answer") + values.size();
    }
    public static int lambdaValue() {
        IntUnaryOperator operation = x -> x + 3;
        return operation.applyAsInt(4);
    }
    public static int capturedLambdaValue() {
        int base = 5;
        IntUnaryOperator operation = x -> x + base;
        return operation.applyAsInt(4);
    }
    public static int caller() {
        return answer();
    }
    public static int arraySum() {
        int[] values = new int[2];
        values[0] = 7;
        values[1] = 5;
        return values[0] + values[1];
    }
    public static int instanceValue() {
        return new Fixture(37).getValue();
    }
    private int getValue() {
        return value;
    }
}
