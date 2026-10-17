interface Delivery {
    void deliver(String cargo);
}

abstract class LogisticsService {
    protected abstract Delivery createTransport();

    public void processDelivery(String cargo) {
        Delivery transport = createTransport();
        System.out.print("Обработка груза: ");
        transport.deliver(cargo);
    }
}

class TruckDelivery implements Delivery {
    @Override
    public void deliver(String cargo) {
        System.out.println("[Машина] Доставка по шоссе: " + cargo);
    }
}

final class RoadLogistics extends LogisticsService {
    @Override
    protected Delivery createTransport() {
        return new TruckDelivery();
    }
}

class TrainDelivery implements Delivery {
    @Override
    public void deliver(String cargo) {
        System.out.println("[Поезд] Доставка по железной дороге: " + cargo);
    }
}

final class RailLogistics extends LogisticsService {
    @Override
    protected Delivery createTransport() {
        return new TrainDelivery();
    }
}

class AirDelivery implements Delivery {
    @Override
    public void deliver(String cargo) {
        System.out.println("[Самолет] Воздушная доставка: " + cargo);
    }
}

final class AirLogistics extends LogisticsService {
    @Override
    protected Delivery createTransport() {
        return new AirDelivery();
    }
}

public class Main {
    public static void main(String[] args) {
        
        LogisticsService service;
        
        if (args.length > 0) {
            service = new RailLogistics();
        } else {
            service = new RoadLogistics();
        }

        service.processDelivery("Коробка с электроникой весом 10кг");

        System.out.println("\n--- Другой заказ ---");
        LogisticsService airService = new AirLogistics();
        airService.processDelivery("Срочный конверт с документами");
    }
}