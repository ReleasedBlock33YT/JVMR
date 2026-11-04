import java.util.ArrayList;
import java.util.HashMap;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.DirectoryStream;
import java.util.zip.ZipFile;
import java.util.zip.ZipEntry;
import java.util.jar.JarFile;
import java.util.jar.JarEntry;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Iterator;
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
    public static int fileExtendedValue() {
        File fixture = new File("loader/build/Fixture.class");
        File[] files = new File("loader/build").listFiles();
        return fixture.getName().equals("Fixture.class")
            && fixture.getParent().equals("loader/build")
            && fixture.toPath().toString().equals("loader/build/Fixture.class")
            && files != null && files.length > 0 ? 1 : 0;
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
    public static int jarValue() throws Exception {
        JarFile jar = new JarFile("loader/build/fixture.jar");
        JarEntry entry = jar.getJarEntry("Fixture.class");
        InputStream input = jar.getInputStream(entry);
        int a = input.read(), b = input.read(), c = input.read(), d = input.read();
        input.close();
        jar.close();
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
    public static int pathValue() {
        Path base = Path.of("loader/build");
        Path file = base.resolve("Fixture.class");
        return file.toString().endsWith("loader/build/Fixture.class")
            && file.getFileName().toString().equals("Fixture.class")
            && file.getParent().equals(base)
            && file.equals(Path.of("loader/build/Fixture.class")) ? 1 : 0;
    }
    public static int nioStreamValue() throws Exception {
        InputStream input = Files.newInputStream(Path.of("loader/build/Fixture.class"));
        int a = input.read(), b = input.read();
        input.close();
        Path outputPath = Path.of("loader/build/jvmr-nio-stream.bin");
        OutputStream output = Files.newOutputStream(outputPath);
        output.write(new byte[] { 4, 5, 6 });
        output.close();
        byte[] bytes = Files.readAllBytes(outputPath);
        return a == 0xca && b == 0xfe && bytes.length == 3 && bytes[2] == 6 ? 1 : 0;
    }
    public static int directoryValue() throws Exception {
        DirectoryStream<Path> stream = Files.newDirectoryStream(Path.of("loader/build"));
        Iterator<Path> iterator = stream.iterator();
        boolean found = false;
        while (iterator.hasNext()) if (iterator.next().getFileName().toString().equals("Fixture.class")) found = true;
        stream.close();
        return found ? 1 : 0;
    }
    public static int nioMutationValue() throws Exception {
        Path directory = Path.of("loader/build/jvmr-mutation");
        Path source = directory.resolve("source.bin");
        Path copy = directory.resolve("copy.bin");
        Files.createDirectories(directory);
        Files.writeString(source, "mutation");
        Files.copy(source, copy);
        boolean copied = Files.readString(copy).equals("mutation");
        Files.deleteIfExists(source);
        Files.move(copy, source);
        boolean moved = Files.exists(source);
        Files.deleteIfExists(source);
        Files.deleteIfExists(directory);
        return copied && moved && !Files.exists(source) ? 1 : 0;
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
    public static int stringTextExtendedValue() {
        String value = "  Fabric  ";
        return (value.trim().equalsIgnoreCase("fabric") ? 1 : 0)
            + ("abc".compareTo("abd") < 0 ? 10 : 0)
            + ("   ".isBlank() ? 100 : 0)
            + ("xy".repeat(2).equals("xyxy") ? 1000 : 0);
    }
    public static int builderLength() {
        return new StringBuilder().append("abc").append(123).toString().length();
    }
    public static int builderRangeValue() {
        return new StringBuilder().append("abcdef", 1, 4).toString().equals("bcd") ? 1 : 0;
    }
    public static int builderExtendedValue() {
        StringBuilder builder = new StringBuilder();
        builder.append('x').append(true).append(':').append(9L).append(':').append(1.5f).append(':').append(2.5d).append(':').append((CharSequence) "ok");
        return builder.toString().equals("xtrue:9:1.5:2.5:ok") ? 1 : 0;
    }
    public static int enumValue() {
        Mode mode = Mode.BETA;
        return mode.ordinal() + (mode.name().equals("BETA") ? 10 : 0)
            + (Mode.values().length == 2 ? 1 : 0)
            + (mode.toString().equals("BETA") ? 100 : 0)
            + (Mode.valueOf("ALPHA").ordinal() == 0 ? 1000 : 0);
    }
    public static int collectionsValue() {
        java.util.ArrayList<String> list = new java.util.ArrayList<>();
        list.add("first");
        list.add("second");
        java.util.HashMap<String, Integer> map = new java.util.HashMap<>();
        map.put("answer", 42);
        int total = 0;
        for (String value : list) {
            total += value.length();
        }
        return list.size() + total + (list.get(1).equals("second") ? 10 : 0)
            + map.get("answer") + (map.containsKey("answer") ? 100 : 0);
    }
    public static int setValue() {
        java.util.HashSet<String> set = new java.util.HashSet<>();
        return (set.add("one") ? 1 : 0) + (set.add("one") ? 10 : 0)
            + (set.contains("one") ? 100 : 0) + set.size();
    }
    public static int collectionMutationValue() {
        java.util.ArrayList<String> list = new java.util.ArrayList<>();
        list.add("old");
        list.set(0, "new");
        int result = list.contains("new") ? 1 : 0;
        result += list.remove(0).equals("new") ? 10 : 0;
        java.util.HashMap<String, Integer> map = new java.util.HashMap<>();
        map.put("x", 7);
        result += map.remove("x");
        java.util.HashSet<String> set = new java.util.HashSet<>();
        set.add("x");
        result += set.remove("x") ? 100 : 0;
        return result;
    }
    public static int optionalValue() {
        java.util.Optional<String> present = java.util.Optional.of("yes");
        java.util.Optional<String> empty = java.util.Optional.empty();
        return (present.isPresent() ? 1 : 0) + (empty.isEmpty() ? 10 : 0)
            + (present.get().equals("yes") ? 100 : 0)
            + (empty.orElse("fallback").equals("fallback") ? 1000 : 0);
    }
    public static int collectionFactoryValue() {
        java.util.List<String> list = java.util.List.of("a", "bb");
        java.util.Set<String> set = java.util.Set.of("x", "y");
        java.util.Map<String, Integer> map = java.util.Map.of("n", 7);
        return list.size() + list.get(1).length() * 10 + set.size() * 100
            + map.get("n");
    }
    public static int collectionsUtilityValue() {
        java.util.List<String> list = java.util.Collections.singletonList("x");
        java.util.Set<String> set = java.util.Collections.singleton("y");
        java.util.Map<String, Integer> map = java.util.Collections.singletonMap("n", 3);
        return list.size() + set.size() * 10 + map.get("n")
            + java.util.Collections.emptyList().size()
            + (java.util.Collections.unmodifiableList(list) == list ? 100 : 0);
    }
    public static int concurrentMapValue() {
        java.util.concurrent.ConcurrentHashMap<String, Integer> map = new java.util.concurrent.ConcurrentHashMap<>();
        map.put("value", 9);
        return map.get("value") + map.size() + (map.containsKey("value") ? 100 : 0);
    }
    public static int atomicValue() {
        java.util.concurrent.atomic.AtomicInteger counter = new java.util.concurrent.atomic.AtomicInteger(4);
        int old = counter.getAndIncrement();
        return old + counter.incrementAndGet() + counter.get();
    }
    public static int recordValue() {
        Point point = new Point(7, 5);
        return point.x() * 10 + point.y();
    }
    public static int objectMethodsValue() {
        Fixture first = new Fixture();
        Fixture second = new Fixture();
        return (first.equals(first) ? 1 : 0)
            + (!first.equals(second) ? 10 : 0)
            + (first.toString().startsWith("Fixture@") ? 100 : 0);
    }
    public static int nioListStreamValue() throws Exception {
        java.util.stream.Stream<Path> stream = Files.list(Path.of("loader/build"));
        Iterator<Path> iterator = stream.iterator();
        int found = 0;
        while (iterator.hasNext()) {
            if (iterator.next().getFileName().toString().equals("Fixture.class")) {
                found++;
            }
        }
        stream.close();
        return found;
    }
    public static int nioWalkValue() throws Exception {
        Path root = Path.of("loader/build/jvmr-walk");
        Path nested = root.resolve("nested");
        Files.createDirectories(nested);
        Path marker = nested.resolve("marker.txt");
        Files.writeString(marker, "walk");
        java.util.stream.Stream<Path> stream = Files.walk(root);
        Iterator<Path> iterator = stream.iterator();
        int found = 0;
        while (iterator.hasNext()) {
            if (iterator.next().getFileName().toString().equals("marker.txt")) {
                found++;
            }
        }
        stream.close();
        Files.deleteIfExists(marker);
        Files.deleteIfExists(nested);
        Files.deleteIfExists(root);
        return found;
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
