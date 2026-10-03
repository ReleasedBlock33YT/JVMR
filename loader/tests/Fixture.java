import java.util.ArrayList;
import java.util.HashMap;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
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
